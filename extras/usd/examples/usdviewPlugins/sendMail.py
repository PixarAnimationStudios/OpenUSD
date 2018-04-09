#
# Copyright 2016 Pixar
#
# Licensed under the Apache License, Version 2.0 (the "Apache License")
# with the following modification; you may not use this file except in
# compliance with the Apache License and the following modification to it:
# Section 6. Trademarks. is deleted and replaced with:
#
# 6. Trademarks. This License does not grant permission to use the trade
#    names, trademarks, service marks, or product names of the Licensor
#    and its affiliates, except as required to comply with Section 4(c) of
#    the License and to reproduce the content of the NOTICE file.
#
# You may obtain a copy of the Apache License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the Apache License with the above modification is
# distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied. See the Apache License for the specific
# language governing permissions and limitations under the Apache License.
#
# Sendmail Plugin
#
# This module provides plug-in facilities for usdview to send emails containing
# captured screenshots from usdview.
#
# Use the `SendMail` function as the callback function for a Usdview command
# plugin. Optionally, modify `_GetSenderAddress` to auto-fill the sender's email
# address.

from pxr.Usdviewq.qt import QtWidgets, QtCore

def _GetSenderAddress():
    """Returns an email address used to auto-fill the 'From' field."""
    return ""

def SendMail(usdviewApi):
    """This method creates a local SMTP server, runs the dialog
       and upon successful completion, sends the info to the intended
       recipient."""
    import smtplib
    import tempfile
    from email.mime.text import MIMEText
    from email.mime.image import MIMEImage
    from email.mime.multipart import MIMEMultipart

    # create the image to send as a temporary file
    with tempfile.NamedTemporaryFile(suffix=".jpeg") as tempimagefile:
        windowShot = usdviewApi.GrabWindowShot()
        viewportShot = usdviewApi.GrabViewportShot()

        # If we can't grab a shot of the viewport (probably if --norender is
        # used) then only allow window shots.
        allowScreenCapSel = (viewportShot is not None)

        # fetch the info from the launched sendmail dialog
        mailInfo = _GetSendMailInfo(usdviewApi, allowScreenCapSel)

        if not _ValidMailInfo(mailInfo):
            return False

        # set the image based on user config
        if mailInfo.imagetype == "Window":
            imagedata = windowShot
        else:
            imagedata = viewportShot

        # verify we created an image correctly and could safely save it
        if not imagedata or not imagedata.save(tempimagefile.name):
            return False

        # construct the message and convert the body to MIMEText
        msg = MIMEMultipart()
        msg['Subject'] = mailInfo.subject
        msg['From'] = (mailInfo.sender.strip() if
                       mailInfo.sender.strip() else None)
        msg['To'] = ','.join(x.strip() for x in mailInfo.sendee.split(',') if
                             x.strip())

        # Either the from or to category was unsatisfactory, bail
        if msg['From'] is None or len(msg['To'].split(',')) == 0:
            return False

        msg.attach(MIMEText(mailInfo.body))
        # grab the image data from the file, convert it to
        # MIME and attach it to the message
        msg.attach(MIMEImage(tempimagefile.read(), _subtype="jpeg"))

        # launch smtp server on local and send mail
        server = smtplib.SMTP('localhost')

        # Note that pythons stmplib requires the second arg as a list here
        # even though msg['To'] requires a string.
        server.sendmail(msg['From'],
                        msg['To'].split(','),
                        msg.as_string())
        server.quit()

class EmailInfo():
    """A class to represent necessary fields in an email"""
    def __init__(self='', sender='', sendee='', subject='', body='',
                 imagetype=''):
        self.sender = sender
        self.sendee = sendee
        self.subject = subject
        self.body = body
        self.imagetype = imagetype

def _GenerateLayout(dialog, allowScreenCapSel):
    """This method generates the full dialog, containing the text fields,
       buttons and combobox for choosing the type of screenshot"""
    (fields, textLayout) = _GenerateTextFields(dialog)
    buttonLayout = _GenerateButtons(dialog, fields, allowScreenCapSel)

    # compose layouts
    layout = QtWidgets.QVBoxLayout()
    layout.addLayout(textLayout)
    layout.addLayout(buttonLayout)

    return layout

def _GenerateTextFields(dialog):
    """This method generates the layout containing the text fields to
        be filled by the user, such as sender, sendee and message.
        Note that this takes a reference to the dialog because the buttons
        need to create a signal handler which uses the text field info"""
    # construct text layout
    senderLabel = QtWidgets.QLabel("From: ")
    sendeeLabel = QtWidgets.QLabel("To: ")
    subjectLabel = QtWidgets.QLabel("Subject: ")
    bodyLabel   = QtWidgets.QLabel("Message: ")

    senderFld = QtWidgets.QLineEdit(dialog.emailInfo.sender)
    sendeeFld = QtWidgets.QLineEdit(dialog.emailInfo.sendee)
    subjectFld = QtWidgets.QLineEdit(dialog.emailInfo.subject)
    bodyFld   = QtWidgets.QTextEdit()
    bodyFld.setText(dialog.emailInfo.body)

    senderLayout = QtWidgets.QHBoxLayout()
    senderLayout.addWidget(senderFld)

    sendeeLayout = QtWidgets.QHBoxLayout()
    sendeeLayout.addWidget(sendeeFld)

    subjectLayout = QtWidgets.QHBoxLayout()
    subjectLayout.addWidget(subjectFld)

    bodyLayout = QtWidgets.QHBoxLayout()
    bodyLayout.addWidget(bodyFld)

    fieldsLayout = QtWidgets.QVBoxLayout()
    fieldsLayout.addWidget(senderLabel)
    fieldsLayout.addLayout(senderLayout)
    fieldsLayout.addWidget(sendeeLabel)
    fieldsLayout.addLayout(sendeeLayout)
    fieldsLayout.addWidget(subjectLabel)
    fieldsLayout.addLayout(subjectLayout)
    fieldsLayout.addWidget(bodyLabel)
    fieldsLayout.addLayout(bodyLayout)

    return ({"sender" : senderFld,
            "sendee" : sendeeFld,
            "subject" :subjectFld,
            "body":    bodyFld},
            fieldsLayout)

def _GenerateButtons(dialog, fields, allowScreenCapSel):
    """This method generates the buttons used in our sendmail
       dialog. This includes creating signal handlers which batch
       the inputted info for sending across the wire."""

    # construct buttons
    cancelBt = QtWidgets.QPushButton("Cancel")
    acceptBt = QtWidgets.QPushButton("Send")

    # construct screen cap selector if there are multiple options available
    screenCapSel = None
    if allowScreenCapSel:
        screenCapSel = QtWidgets.QComboBox()
        screenCapSel.setSizeAdjustPolicy(
                QtWidgets.QComboBox.AdjustToContents)
        screenCapSel.addItem("Window")
        screenCapSel.addItem("Viewport")

    buttonLayout = QtWidgets.QHBoxLayout()
    buttonLayout.addStretch(0.5)
    if allowScreenCapSel:
        buttonLayout.addWidget(screenCapSel)
    buttonLayout.addWidget(cancelBt)
    buttonLayout.addWidget(acceptBt)

    # add a signal handler for the accept and close buttons
    # upon cancelling, simply close the dialog
    cancelBt.clicked.connect(lambda : dialog.close())

    # upon accepting, fill the data and close the dialog
    acceptBt.clicked.connect(lambda : _FillDataFromDialog(
         dialog,
         fields,
         screenCapSel.currentText() if allowScreenCapSel else "Window").close())
    return buttonLayout

def _GetSendMailInfo(usdviewApi, allowScreenCapSel):
    """This method takes the generated dialog, runs it and returns the
       inputted information upon the dialogs completion"""
    window = usdviewApi.qMainWindow
    dialog = QtWidgets.QDialog(window)
    dialog.setWindowTitle("Send Screenshot...")

    # set field defaults
    dialog.emailInfo = _GenerateDefaultInfo(usdviewApi, dialog)
    dialog.setMinimumWidth((window.size().width()/2))

    # add layout to dialog and launch
    dialog.setLayout(_GenerateLayout(dialog, allowScreenCapSel))
    dialog.exec_()

    return dialog.emailInfo

def _GenerateDefaultInfo(usdviewApi, dialog):
    """This method generates default information for the sender and
       bodyfields of the sendmail dialog. For the sender, it calls the
       _GetSenderAddress function. For the body, it generates some
       information based on the current UsdStage."""
    from time import strftime, gmtime
    import os

    currtime = strftime("%b %d, %Y, %H:%M:%S", gmtime())

    # if a user is defined, we'll use that
    sender = _GetSenderAddress()

    mainWindow = usdviewApi.qMainWindow
    camera = usdviewApi.currentGfCamera
    sendee = ""
    subject = 'Usdview Screenshot'
    body = str("Usdview screenshot, taken " + currtime + "\n" +
               "----------------------------------------" + "\n" +
               "File: "
                  + str(usdviewApi.stageIdentifier) + "\n" +
               "Selected Prim Paths: "
                  + ", ".join(map(str, usdviewApi.selectedPrims)) + "\n" +
               "Current Frame: "
                  + str(usdviewApi.frame) + "\n" +
               "Complexity: "
                  + str(usdviewApi.dataModel.viewSettings.complexity) + "\n" +
               "Camera Info:\n" +str(camera) + "\n" +
               "----------------------------------------" + "\n")

    return EmailInfo(sender, sendee, subject, body)

def _FillDataFromDialog(dialog, fields, imagetype):
    """This method simply fills members of the dialog object"""
    dialog.emailInfo.sender = fields["sender"].text()
    dialog.emailInfo.sendee = fields["sendee"].text()
    dialog.emailInfo.subject = fields["subject"].text()
    dialog.emailInfo.body = fields["body"].toPlainText()
    dialog.emailInfo.imagetype = imagetype
    return dialog

def _ValidMailInfo(mailInfo):
    """This method asserts that the fields are non empty."""
    return (mailInfo.sender  and \
            mailInfo.sendee  and \
            mailInfo.subject and \
            mailInfo.body    )
