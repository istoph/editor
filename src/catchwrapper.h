// SPDX-License-Identifier: BSL-1.0

#ifndef CATCHWRAPPER_H
#define CATCHWRAPPER_H


#include <QString>
#include <QRect>

#include <Tui/ZColor.h>
#include <Tui/ZDocument.h>

#ifdef CATCH3
#include <catch2/catch_all.hpp>
using Catch::Approx;
#else
#include <catch2/catch.hpp>
#endif

#include "filecategorize.h"

namespace Catch {
    template<>
    struct StringMaker<QString, void> {
        static std::string convert(QString const& value) {
            return "\"" + value.toStdString() + "\"";
        }
    };

    template<>
    struct StringMaker<QRect, void> {
        static std::string convert(QRect const& value) {
            return QStringLiteral("(x: %0, y: %1, w: %2, h: %3)").arg(
                        QString::number(value.x()),
                        QString::number(value.y()),
                        QString::number(value.width()),
                        QString::number(value.height())).toStdString();
        }
    };

    template<>
    struct StringMaker<QPoint, void> {
        static std::string convert(QPoint const& value) {
            return QStringLiteral("(x: %0, y: %1)").arg(
                        QString::number(value.x()),
                        QString::number(value.y())).toStdString();
        }
    };

    template<>
    struct StringMaker<Tui::ZDocumentCursor::Position, void> {
        static std::string convert(Tui::ZDocumentCursor::Position const& value) {
            return QStringLiteral("(codeUnit: %0, line: %1)").arg(
                        QString::number(value.codeUnit),
                        QString::number(value.line)).toStdString();
        }
    };


    template<>
    struct StringMaker<Tui::ZColor, void> {
        static std::string convert(Tui::ZColor const& value) {
            if (value.colorType() == Tui::ZColor::RGB) {
                return QStringLiteral("(color: rgb(%0, %1, %2))").arg(QString::number(value.red()),
                                                                      QString::number(value.green()),
                                                                      QString::number(value.blue())).toStdString();
            } else if (value.colorType() == Tui::ZColor::Default) {
                return "(color: default)";
            } else if (value.colorType() == Tui::ZColor::Terminal) {
                return QStringLiteral("(color: %0)").arg(QString::number(static_cast<int>(value.terminalColor()))).toStdString();
            } else if (value.colorType() == Tui::ZColor::TerminalIndexed) {
                return QStringLiteral("(color indexed: %0)").arg(QString::number(value.terminalColorIndexed())).toStdString();
            } else {
                return "(invalid color)";
            }
        }
    };

    template<>
    struct StringMaker<FileCategory, void> {
        static std::string convert(FileCategory const& value) {
            if (value == FileCategory::dir) {
                return "dir";
            } else if (value == FileCategory::stdin_file) {
                return "stdin_file";
            } else if (value == FileCategory::new_file) {
                return "new_file";
            } else if (value == FileCategory::open_file) {
                return "open_file";
            } else if (value == FileCategory::invalid_error) {
                return "invalid_error";
            } else if (value == FileCategory::invalid_filetype) {
                return "invalid_filetype";
            } else if (value == FileCategory::invalid_dir_not_exist) {
                return "invalid_dir_not_exist";
            } else if (value == FileCategory::invalid_dir_not_writable) {
                return "invalid_dir_not_writable";
            }
            return "not exist";
        }
    };
}

#endif
