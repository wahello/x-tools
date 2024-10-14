﻿/***************************************************************************************************
 * Copyright 2024 x-tools-author(x-tools@outlook.com). All rights reserved.
 *
 * The file is encoded using "utf8 with bom", it is a part of eTools project.
 *
 * eTools is licensed according to the terms in the file LICENCE(GPL V3) in the root of the source
 * code directory.
 **************************************************************************************************/
#pragma once

#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>

namespace Ui {
class ChartsUiSettings;
}

namespace xTools {

class ChartsUiSettings : public QWidget
{
    Q_OBJECT
public:
    enum SeriesType { LineSeries, SplineSeries, ScatterSeries };

public:
    explicit ChartsUiSettings(QWidget *parent = Q_NULLPTR);
    ~ChartsUiSettings() override;

    static int channelCount();

private:
    struct ChannelContext
    {
        QCheckBox *visibleCheckBox;
        QComboBox *typeComboBox;
        QPushButton *colorButton;
        QLineEdit *nameLineEdit;
    };
    Ui::ChartsUiSettings *ui;
    QList<ChannelContext> m_channelContexts;

private:
    void setupVisibleCheckBox(QCheckBox *checkBox, int channelIndex);
    void setupTypeComboBox(QComboBox *comboBox, int channelIndex);
    void setupColorButton(QPushButton *button, int channelIndex);
    void setupNameLineEdit(QLineEdit *lineEdit, int channelIndex);

    QString seriesTypeToString(int type) const;
};

} // namespace xTools
