/*
 S PDX-FileCopyrightText: 2026 Alex Shek <hms.starryfish@gmail.com>

 SPDX-License-Identifier: Apache-2.0
 */
#include "yaftirunner.h"

#include <QFile>
#include <KLocalizedString>
#include <QProcess>
#include <QStandardPaths>
#include <QDebug>
#include <yaml-cpp/yaml.h>

YaftiRunner::YaftiRunner(QObject *parent, const KPluginMetaData &metaData)
: KRunner::AbstractRunner(parent, metaData)
{
    setObjectName(QStringLiteral("Yafti"));
    loadActions();
}

YaftiRunner::~YaftiRunner() = default;

void YaftiRunner::loadActions()
{
    const QString yamlPath = QStringLiteral("/usr/share/yafti/yafti.yml");
    if (!QFile::exists(yamlPath)) {
        qWarning() << "Yafti config not found:" << yamlPath;
        return;
    }

    try {
        YAML::Node config = YAML::LoadFile(yamlPath.toStdString());
        if (!config["screens"]) return;

        for (const auto &screenNode : config["screens"]) {
            if (screenNode["hidden"] && screenNode["hidden"].as<bool>(false)) {
                continue;
            }
            QString screenTitle = QString::fromStdString(screenNode["title"].as<std::string>(""));
            if (!screenNode["actions"]) continue;
            for (const auto &actionNode : screenNode["actions"]) {
                if (actionNode["hidden"] && actionNode["hidden"].as<bool>(false)) {
                    continue;
                }
                Action act;
                act.id = QString::fromStdString(actionNode["id"].as<std::string>(""));
                act.title = QString::fromStdString(actionNode["title"].as<std::string>(""));
                act.description = QString::fromStdString(actionNode["description"].as<std::string>(""));
                act.screenTitle = screenTitle;
                if (!act.id.isEmpty())
                    m_actions.append(act);
            }
        }
    } catch (const YAML::Exception &e) {
        qWarning() << "Failed to parse YAFTI YAML:" << e.what();
    }
}

void YaftiRunner::match(KRunner::RunnerContext &context)
{
    const QString query = context.query().trimmed();
    if (query.isEmpty())
        return;

    int titleMatches = 0;
    int descMatches = 0;

    for (const Action &act : m_actions) {
        const bool isTitleMatch = act.title.contains(query, Qt::CaseInsensitive);
        const bool isDescMatch = act.description.contains(query, Qt::CaseInsensitive);
        if (!isTitleMatch && !isDescMatch) {
            continue;
        }

        KRunner::QueryMatch match(this);
        match.setId(act.id);
        match.setText(act.title);
        match.setSubtext(act.description + QStringLiteral(" (") + act.screenTitle + QStringLiteral(")"));
        match.setIconName(QStringLiteral("io.github.ublue_os.yafti_gtk"));
        if (isTitleMatch) {
            const double relevance = 0.85 - (titleMatches * 0.005);
            match.setRelevance(qMax(relevance, 0.7));
            titleMatches++;
        }
        if (isDescMatch) {
            const double relevance = 0.55 - (descMatches * 0.005);
            match.setRelevance(qMax(relevance, 0.3));
            descMatches++;
        }
        context.addMatch(match);
    }
}

void YaftiRunner::run(const KRunner::RunnerContext &/*context*/, const KRunner::QueryMatch &match)
{
    QString actionId = match.id();
    if (actionId.isEmpty())
        return;

    // Remove the prefix "krunner-yafti_" if it exists
    if (actionId.startsWith(QStringLiteral("krunner-yafti_"))) {
        actionId.remove(0, 14); // 14 is the length of "krunner-yafti_"
    }

    // Build command: launch yafti_gtk.py with the config file and the action ID
    QString scriptPath = QStringLiteral("/usr/bin/yafti_gtk.py");
    // If not there, try to find it in PATH or fallback to current dir
    if (!QFile::exists(scriptPath)) {
        // Try to locate via which
        const QString which = QStandardPaths::findExecutable(QStringLiteral("yafti_gtk.py"));
        if (!which.isEmpty())
            scriptPath = which;
        else {
            qWarning() << "yafti_gtk.py not found";
            return;
        }
    }

    QStringList args;
    args << QStringLiteral("/usr/share/yafti/yafti.yml") << QStringLiteral("--action-id") << actionId;

    // Launch the Python script as a detached process
    QProcess::startDetached(scriptPath, args);
}

K_PLUGIN_CLASS_WITH_JSON(YaftiRunner, "metadata.json")

#include "yaftirunner.moc"
