/*
 S PDX-FileCopyrightText: 2026 Alex Shek <hms.starryfish@gmail.com>

 SPDX-License-Identifier: Apache-2.0
 */

#include "yaftirunner.h"

#include <KLocalizedString>
#include <QProcess>
#include <QStandardPaths>
#include <QDebug>

YaftiRunner::YaftiRunner(QObject *parent, const KPluginMetaData &data)
: KRunner::AbstractRunner(parent, data)
{
    qDebug() << "YaftiRunner: Constructor called";
    qDebug() << "YaftiRunner: Plugin name:" << data.name();
    qDebug() << "YaftiRunner: Plugin ID:" << data.pluginId();


    setMinLetterCount(2);

    if (!m_yaftiClient.isConnected()) {
        qWarning() << "YaftiRunner: Failed to connect to Bazzite Portal:" << m_yaftiClient.lastError();
    } else {
        qDebug() << "YaftiRunner: Successfully initialized Bazzite Portal";
    }

    addSyntax(QStringLiteral(":q:"), i18n("Search for Flatpak applications in Bazaar"));
    qDebug() << "BazaarRunner: Constructor completed successfully";
}

void YaftiRunner::match(KRunner::RunnerContext &context)
{
    const QString term = context.query();

    if (term.length() < 2) {
        return;
    }

    QList<AppSuggestion> results = m_yaftiClient.search(term, [&context](){
        return context.isValid();
    });

    int addedMatches = 0;
    for (const auto &action : results) {
        if (!context.isValid()) {
            break;
        }

        KRunner::QueryMatch match(this);
        match.setIconName(action.iconName);
        match.setText(i18n("run %1", action.name));
        match.setSubtext(action.description);
        match.setData(action.id);

        // Preserve Bazaar's ordering
        const double relevance = 1.0 - (addedMatches * 0.01);
        match.setRelevance(qMax(relevance, 0.1));

        context.addMatch(match);
        addedMatches++;

        qDebug() << "YaftiRunner::match: Added match for:" << action.name << "with relevance:" << relevance;
    }
}

void YaftiRunner::run(const KRunner::RunnerContext &context, const KRunner::QueryMatch &match)
{
    qDebug() << "YaftiRunner::run called";

    const QString actionId = match.data().toString();
    if (actionId.isEmpty()) {
        qWarning() << "YaftiRunner::run: No action ID provided for execution";
        return;
    }

    qDebug() << "YaftiRunner::run: Activating Bazzite Portal for action ID:" << actionId;

    QStringList terms = context.query().split(QLatin1Char(' '), Qt::SkipEmptyParts);
    bool success = m_yaftiClient.activateResult(actionId, terms);

    if (!success) {
        qWarning() << "Failed to activate result in Bazzite Portal:" << m_yaftiClient.lastError();
    } else {
        qDebug() << "Successfully activated result:" << actionId << "in Bazzite Portal";
    }
}

K_PLUGIN_CLASS_WITH_JSON(YaftiRunner, "yaftirunner.json")

// needed for the QObject subclass declared as part of K_PLUGIN_CLASS_WITH_JSON
#include "yaftirunner.moc"

#include "moc_yaftirunner.cpp"
