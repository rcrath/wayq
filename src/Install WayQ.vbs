' WayQ Windows Installer — double-click to install
' Copies win\WayQ.vst3  to C:\Program Files\Common Files\VST3\
'        win\WayQ.clap  to C:\Program Files\Common Files\CLAP\

Set fso = CreateObject("Scripting.FileSystemObject")
Set shell = CreateObject("WScript.Shell")

myDir   = fso.GetParentFolderName(WScript.ScriptFullName)
srcVst3 = myDir & "\win\WayQ.vst3"
srcClap = myDir & "\win\WayQ.clap"
destVst3Dir = "C:\Program Files\Common Files\VST3"
destClapDir = "C:\Program Files\Common Files\CLAP"
destVst3 = destVst3Dir & "\WayQ.vst3"
destClap = destClapDir & "\WayQ.clap"

If Not fso.FolderExists(srcVst3) Then
    MsgBox "Cannot find win\WayQ.vst3 next to this installer." & vbCrLf & vbCrLf & _
           "You may be running this from inside the zip file." & vbCrLf & _
           "Please extract ALL files from the zip to a folder first," & vbCrLf & _
           "then run Install WayQ.vbs from that folder.", vbCritical, "WayQ Installer"
    WScript.Quit
End If

' Show license (EULA) and require acceptance (only before elevation)
If Not WScript.Arguments.Named.Exists("elevated") Then
    eulaFile = myDir & "\EULA.md"
    If fso.FileExists(eulaFile) Then
        shell.Run "notepad.exe """ & eulaFile & """", 1, False
        result = MsgBox("Please read the WayQ License Agreement that just opened in Notepad." & vbCrLf & vbCrLf & _
                        "WayQ is free software under the GNU GPL v3." & vbCrLf & _
                        "Do you accept the terms?", _
                        vbYesNo + vbQuestion, "WayQ License Agreement")
        If result <> vbYes Then
            WScript.Quit
        End If
    Else
        MsgBox "Cannot find EULA.md next to this installer.", vbCritical, "WayQ Installer"
        WScript.Quit
    End If

    ' Need admin to write to Program Files — relaunch elevated
    Set app = CreateObject("Shell.Application")
    app.ShellExecute "wscript.exe", """" & WScript.ScriptFullName & """ /elevated", "", "runas", 1
    WScript.Quit
End If

' --- Elevated: install VST3 ---
If Not fso.FolderExists(destVst3Dir) Then fso.CreateFolder destVst3Dir
If fso.FolderExists(destVst3) Then fso.DeleteFolder destVst3, True
fso.CopyFolder srcVst3, destVst3

' --- Elevated: install CLAP (if present) ---
clapOk = True
If fso.FileExists(srcClap) Or fso.FolderExists(srcClap) Then
    If Not fso.FolderExists(destClapDir) Then fso.CreateFolder destClapDir
    If fso.FileExists(destClap) Then fso.DeleteFile destClap, True
    If fso.FolderExists(destClap) Then fso.DeleteFolder destClap, True
    ' WayQ.clap is a single file on Windows
    If fso.FileExists(srcClap) Then
        fso.CopyFile srcClap, destClap
        clapOk = fso.FileExists(destClap)
    Else
        fso.CopyFolder srcClap, destClap
        clapOk = fso.FolderExists(destClap)
    End If
End If

If fso.FolderExists(destVst3) And clapOk Then
    MsgBox "WayQ installed!" & vbCrLf & "Restart your DAW to see it.", vbInformation, "WayQ Installer"
Else
    MsgBox "Installation failed. Try running as Administrator.", vbCritical, "WayQ Installer"
End If
