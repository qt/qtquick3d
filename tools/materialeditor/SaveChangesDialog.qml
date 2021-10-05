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

import QtQuick
import QtQuick.Controls
import MaterialEditorHelpers

Dialog {
    id: root
    title: qsTr("Unsaved changes")
    modal: true

    required property MaterialAdapter materialAdapter
    required property var saveAsDialog

    function doIfChangesSavedOrDiscarded(actionFunction) {
        if (!materialAdapter.unsavedChanges) {
            actionFunction()
            return
        }

        // There are unsaved changes, so we need to prompt.

        function disconnectSaveChangesSignals() {
            root.accepted.disconnect(saveChanges)
            root.discarded.disconnect(discardChanges)
            root.rejected.disconnect(cancel)
        }

        function saveChanges() {
            if (materialAdapter.materialSaveFile.toString().length > 0) {
                // Existing project; can save without a dialog.
                if (materialAdapter.save()) {
                    // Saved successfully, so now we can perform the action.
                    performAction()
                } else {
                    // Failed to save; cancel.
                    cancel()
                }
            } else {
                // New project; need to save as.
                function disconnectSaveAsSignals() {
                    materialAdapter.errorOccurred.disconnect(saveAsFailed)
                    materialAdapter.postMaterialSaved.disconnect(saveAsSucceeded)
                    saveAsDialog.rejected.disconnect(saveAsDialogRejected)
                }

                function saveAsSucceeded() {
                    disconnectSaveAsSignals()
                    performAction()
                }

                function saveAsFailed() {
                    disconnectSaveAsSignals()
                    disconnectSaveChangesSignals()
                }

                function saveAsDialogRejected() {
                    disconnectSaveAsSignals()
                    cancel()
                }

                materialAdapter.errorOccurred.connect(saveAsFailed)
                materialAdapter.postMaterialSaved.connect(saveAsSucceeded)
                saveAsDialog.rejected.connect(saveAsDialogRejected)

                saveAsDialog.open()
            }
        }

        function discardChanges() {
            performAction()
            root.close()
        }

        function performAction() {
            disconnectSaveChangesSignals()
            actionFunction()
        }

        function cancel() {
            disconnectSaveChangesSignals()
        }

        root.accepted.connect(saveChanges)
        root.discarded.connect(discardChanges)
        root.rejected.connect(cancel)
        root.open()
    }

    Label {
        text: qsTr("Save changes to the material before closing?")
    }

    // Using a DialogButtonBox allows us to assign objectNames to the buttons,
    // which makes it possible to test them.
    footer: DialogButtonBox {
        Button {
            objectName: "cancelDialogButton"
            text: qsTr("Cancel")
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
        }
        Button {
            objectName: "saveChangesDialogButton"
            text: qsTr("Save")
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
        }
        Button {
            objectName: "discardChangesDialogButton"
            text: qsTr("Don't save")
            DialogButtonBox.buttonRole: DialogButtonBox.DestructiveRole
        }
    }
}
