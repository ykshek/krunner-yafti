#include "yafticlient.h"

#include <QDBusConnection>
#include <QDBusReply>
#include <QDBusMessage>
#include <QDBusArgument>
#include <QDBusVariant>
#include <QDebug>
#include <QDateTime>

using namespace Qt::Literals::StringLiterals;

// TODO: make this configurable in a KCM
static constexpr int kMaxNumResults = 6;

QString kDBusServiceName = QStringLiteral("io.github.ublue_os.yafti_gtk");
QString kDBusServicePath = QStringLiteral("/io/github/ublue_os/yafti/SearchProvider");
QString kDBusServiceInterface = QStringLiteral("org.gnome.Shell.SearchProvider2");

struct ResultMetas {
    QList<QVariantMap> metas;
};

Q_DECLARE_METATYPE(ResultMetas)

const QDBusArgument& operator>>(const QDBusArgument& arg, ResultMetas& metas)
{
    // Start parsing aa{sv}
    arg.beginArray();

    int elementIndex = 0;
    while (!arg.atEnd()) {
        qDebug() << "Processing element" << elementIndex++ << "with type:" << arg.currentType();

        QVariantMap meta;
        arg.beginMap();

        while (!arg.atEnd()) {
            if (arg.currentType() != QDBusArgument::MapEntryType) {
                qWarning() << "  Expected MapEntryType but got:" << arg.currentType();
                break;
            }

            arg.beginMapEntry();

            QString key;
            arg >> key;

            QDBusVariant dbusVariant;
            arg >> dbusVariant;
            QVariant value = dbusVariant.variant();

            arg.endMapEntry();

            meta[key] = value;
        }
        arg.endMap();

        metas.metas.append(meta);
        qDebug() << "  Added metadata map with" << meta.size() << "entries";
    }

    arg.endArray();
    return arg;
}

YaftiClient::YaftiClient() {
    m_yaftiInterface = std::make_unique<QDBusInterface>(
        kDBusServiceName,
        kDBusServicePath,
        kDBusServiceInterface,
        QDBusConnection::sessionBus()
    );

    if (!m_yaftiInterface->isValid()) {
        m_lastError = m_yaftiInterface->lastError().message();
        qWarning() << "YaftiClient: failed to connect to Bazzite Portal D-Bus service " << kDBusServiceName << ": " << m_lastError;
        qWarning() << "YaftiClient: Make sure Bazzite Portal is running and the search provider is enabled.";
    } else {
        qDebug() << "YaftiClient: successfully connected to Bazzite Portal D-Bus service " << kDBusServiceName;
        m_lastError.clear();
    }
}

bool YaftiClient::isConnected() const {
    return m_yaftiInterface && m_yaftiInterface->isValid();
}

QString YaftiClient::lastError() const {
    return m_lastError;
}

QList<AppSuggestion> YaftiClient::search(const QString &term, const std::function<bool()> &isContextValid) {
    QList<AppSuggestion> results;

    if (!isConnected()) {
        m_lastError = "Not connected to Bazzite Portal D-Bus interface"_L1;
        qDebug() << "YaftiClient::search:" << m_lastError;
        return results;
    }

    if (term.length() < 2) {
        m_lastError = "Search term too short (minimum 2 characters)"_L1;
        return results;
    }

    // Split search term into individual terms
    QStringList terms = term.split(QLatin1Char(' '), Qt::SkipEmptyParts);

    if (isContextValid && !isContextValid()) {
        return results;
    }

    // Get initial result set
    QStringList resultIds = getInitialResultSet(terms);
    if (resultIds.isEmpty()) {
        qDebug() << "YaftiClient::search: No results returned from Bazzite Portal for query:" << term;
        return results;
    }

    if (isContextValid && !isContextValid()) {
        return results;
    }

    qDebug() << "YaftiClient::search: Bazzite Portal returned" << resultIds.size() << "result IDs (will be truncated to " << kMaxNumResults << ")";

    if (resultIds.size() > kMaxNumResults) {
        resultIds = resultIds.mid(0, kMaxNumResults);
    }

    QList<QVariantMap> metas = getResultMetas(resultIds);

    // Extract metadata for each result
    for (int i = 0; i < resultIds.size() && i < metas.size(); ++i) {
        if (isContextValid && !isContextValid()) {
            break;
        }

        const QVariantMap &meta = metas[i];

        AppSuggestion suggestion;
        suggestion.id = resultIds[i];
        suggestion.name = meta.value(QStringLiteral("name")).toString();
        suggestion.description = meta.value(QStringLiteral("description")).toString();

        suggestion.iconName = meta.value(QStringLiteral("gicon")).toString();
        if (suggestion.iconName.isEmpty()) {
            // Handle serialized icon variant
            QVariant iconVariant = meta.value(QStringLiteral("icon"));
            if (iconVariant.isValid()) {
                // For now, use a fallback icon since handling serialized GIcon is complex
                suggestion.iconName = QStringLiteral("application-x-flatpak");
            }
        }

        // If no icon is provided, use a default Flatpak icon
        if (suggestion.iconName.isEmpty()) {
            suggestion.iconName = QStringLiteral("application-x-flatpak");
        }

        // Skip if we couldn't get a name
        if (suggestion.name.isEmpty()) {
            qWarning() << "YaftiClient::search: Skipping result with empty name:" << suggestion.id;
            continue;
        }

        results.append(suggestion);
    }

    return results;
}

bool YaftiClient::activateResult(const QString &appId, const QStringList &searchTerms) {
    if (!isConnected()) {
        m_lastError = "Not connected to Bazzite Portal D-Bus interface"_L1;
        qWarning() << "YaftiClient::activateResult:" << m_lastError;
        return false;
    }

    qDebug() << "YaftiClient::activateResult: Activating app ID:" << appId;

    uint timestamp = static_cast<uint>(QDateTime::currentSecsSinceEpoch());

    QDBusReply<void> reply = m_yaftiInterface->call(
        QStringLiteral("ActivateResult"),
                                                     appId,
                                                     searchTerms,
                                                     timestamp
    );

    if (!reply.isValid()) {
        m_lastError = reply.error().message();
        qWarning() << "YaftiClient::activateResult: Failed to activate result:" << m_lastError;
        return false;
    }

    return true;
}

QStringList YaftiClient::getInitialResultSet(const QStringList &terms) {
    QDBusReply<QStringList> reply = m_yaftiInterface->call(QStringLiteral("GetInitialResultSet"), terms);

    if (!reply.isValid()) {
        m_lastError = reply.error().message();
        qWarning() << "YaftiClient::getInitialResultSet: Failed to get search results:" << m_lastError;
        return QStringList();
    }

    return reply.value();
}

QList<QVariantMap> YaftiClient::getResultMetas(const QStringList &resultIds) {
    ResultMetas metas;

    QDBusMessage metaCall = QDBusMessage::createMethodCall(
        kDBusServiceName,
        kDBusServicePath,
        kDBusServiceInterface,
        QStringLiteral("GetResultMetas")
    );

    metaCall << resultIds;

    QDBusMessage metaReply = QDBusConnection::sessionBus().call(metaCall);

    if (metaReply.type() == QDBusMessage::ErrorMessage) {
        m_lastError = metaReply.errorMessage();
        qWarning() << "YaftiClient::getResultMetas: Failed to get result metadata:" << m_lastError;
        return {};
    }


    if (metaReply.arguments().isEmpty()) {
        m_lastError = "No arguments in GetResultMetas reply"_L1;
        qWarning() << "YaftiClient::getResultMetas:" << m_lastError;
        return {};
    }

    // Get the first argument which should be aa{sv}
    QVariant metaVariant = metaReply.arguments().at(0);
    qDebug() << "YaftiClient::getResultMetas: Metadata variant type:" << metaVariant.typeName();

    QDBusArgument metaArg = metaVariant.value<QDBusArgument>();
    metaArg >> metas;

    return metas.metas;
}
