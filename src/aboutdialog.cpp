// SPDX-License-Identifier: BSL-1.0

#include "aboutdialog.h"

#include <Tui/ZButton.h>
#include <Tui/ZLabel.h>
#include <Tui/ZHBoxLayout.h>
#include <Tui/ZVBoxLayout.h>

#include <QCoreApplication>

AboutDialog::AboutDialog(Tui::ZWidget *parent) : Tui::ZDialog(parent) {
    setOptions(Tui::ZWindow::CloseOption | Tui::ZWindow::MoveOption | Tui::ZWindow::AutomaticOption);
    setFocus();
    setWindowTitle("About chr");
    setContentsMargins({ 1, 1, 1, 1});

    Tui::ZVBoxLayout *vbox = new Tui::ZVBoxLayout();
    setLayout(vbox);
    vbox->setSpacing(1);

    Tui::ZLabel *nameLabel = new Tui::ZLabel(this);
    nameLabel->setText(QCoreApplication::applicationVersion());
    vbox->addWidget(nameLabel);

    Tui::ZLabel *authorLabel = new Tui::ZLabel(this);
    authorLabel->setText("Authors: Christoph Hüffelmann and Martin Hostettler");
    vbox->addWidget(authorLabel);

    Tui::ZLabel *licenseLabel = new Tui::ZLabel(this);
    licenseLabel->setText("License: Boost Software License - Version 1.0");
    vbox->addWidget(licenseLabel);

#ifdef SYNTAX_HIGHLIGHTING
    Tui::ZLabel *moduleLabel = new Tui::ZLabel(this);
    moduleLabel->setText("Module: SyntaxHighlighting");
    vbox->addWidget(moduleLabel);
#endif

    Tui::ZLabel *githubLabel = new Tui::ZLabel(this);
    githubLabel->setText("Bugtracker: https://github.com/istoph/editor");
    vbox->addWidget(githubLabel);

    Tui::ZHBoxLayout *hbox1 = new Tui::ZHBoxLayout();
    hbox1->addStretch();
    Tui::ZButton *okButton = new Tui::ZButton(this);
    okButton->setText("OK");
    okButton->setDefault(true);
    hbox1->addWidget(okButton);
    vbox->add(hbox1);

    QObject::connect(okButton, &Tui::ZButton::clicked, this, &AboutDialog::deleteLater);
}
