/*
 *  SPDX-FileCopyrightText: 2026 Alex Shek <hms.starryfish@gmail.com>
 *
 *  SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <KRunner/AbstractRunner>
#include <QList>

struct Action {
    QString id;
    QString title;
    QString description;
    QString screenTitle;
};

class YaftiRunner : public KRunner::AbstractRunner
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.plasma.Runner")
public:
    YaftiRunner(QObject *parent, const KPluginMetaData &metaData);
    ~YaftiRunner() override;

    void match(KRunner::RunnerContext &context) override;
    void run(const KRunner::RunnerContext &context, const KRunner::QueryMatch &match) override;

private:
    QList<Action> m_actions;
    void loadActions();
};
