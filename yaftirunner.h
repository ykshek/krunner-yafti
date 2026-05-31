/*
 *  SPDX-FileCopyrightText: 2026 Alex Shek <hms.starryfish@gmail.com>
 *
 *  SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <KRunner/AbstractRunner>
#include "yafticlient.h"

class YaftiRunner : public KRunner::AbstractRunner {
    Q_OBJECT

public:
    YaftiRunner(QObject *parent, const KPluginMetaData &data);

    void match(KRunner::RunnerContext &context) override;
    void run(const KRunner::RunnerContext &context, const KRunner::QueryMatch &match) override;

    YaftiClient m_yaftiClient;
};
