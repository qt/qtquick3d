/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Quick 3D.
**
** $QT_BEGIN_LICENSE:GPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "settingstab.h"

#include <QVBoxLayout>
#include <QtQuick3DAssetImport/private/qssgassetimportmanager_p.h>
#include <QtWidgets>

SettingsTab::SettingsTab(QWidget *parent) : QScrollArea(parent)
{
    QWidget *central = new QWidget;
    QVBoxLayout *mainLayout = new QVBoxLayout(central);

    const auto allDescs = QSSGAssetImportManager().getImporterPluginInfos();

    for (const auto &plugin : allDescs) {
        auto options = plugin.importOptions.value("options").toMap();

        // Skip plugins without options
        if (options.empty())
            continue;

        QGroupBox *pluginGroup = new QGroupBox(plugin.typeDescription + " (" + plugin.name + ")");
        QFormLayout *extensionsLayout = new QFormLayout;
        extensionsLayout->addRow(new QLabel(tr("Supported extensions:")), new QLabel(plugin.inputExtensions.join(", ")));

        for (auto kv_it = options.constKeyValueBegin(); kv_it != options.constKeyValueEnd(); ++kv_it) {
            auto map = kv_it->second.toMap();
            auto description = map.value("description");
            auto name = map.value("name");
            auto type = map.value("type");
            auto value = map.value("value");

            auto label = new QLabel(name.toString() + ":");
            label->setToolTip(description.toString());

            if (type == "Boolean") {
                QCheckBox *checkBox = new QCheckBox();
                checkBox->setChecked(value.toBool());
                extensionsLayout->addRow(label, checkBox);
                settings.push_back(Setting { checkBox, kv_it->first, value.toBool(), 0.0f });
            } else if (type == "Real") {
                QSpinBox *spinBox = new QSpinBox();
                spinBox->setMinimum(-9999);
                spinBox->setMaximum(9999);
                spinBox->setSingleStep(1);
                spinBox->setValue(value.toReal());
                extensionsLayout->addRow(label, spinBox);
                settings.push_back(Setting { spinBox, kv_it->first, false, value.toReal() });
            } else {
                qWarning() << "Unsupported setting " << name;
            }
        }

        pluginGroup->setLayout(extensionsLayout);
        mainLayout->addWidget(pluginGroup);
    }

    mainLayout->addStretch(1);

    setWidget(central);
    setWidgetResizable(true);
}

QVariantMap SettingsTab::getOptions() const
{
    QVariantMap options;

    for (const Setting &setting : settings) {
        auto checkBox = dynamic_cast<QCheckBox *>(setting.uiELement);
        auto spinBox = dynamic_cast<QSpinBox *>(setting.uiELement);

        if (checkBox != nullptr && setting.defaultBool != checkBox->isChecked()) {
            options[setting.name] = QVariant::fromValue(checkBox->isChecked());
        } else if (spinBox != nullptr && setting.defaultReal != spinBox->value()) {
            options[setting.name] = QVariant::fromValue(double(spinBox->value()));
        }
    }

    return options;
}
