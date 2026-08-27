' opc_cmd.vbs -- OPC DA helper for CRASHOVERRIDE demo
' Usage:
'   cscript //nologo opc_cmd.vbs <ProgID> WRITE <tag> <value>
'   cscript //nologo opc_cmd.vbs <ProgID> READ  <tag>
'
' Output: OK or OK:<value> or ERROR:<message>

On Error Resume Next

If WScript.Arguments.Count < 3 Then
    WScript.Echo "ERROR:Usage: opc_cmd.vbs <ProgID> READ|WRITE <tag> [value]"
    WScript.Quit 1
End If

Dim progId, action, tag
progId = WScript.Arguments(0)
action = UCase(WScript.Arguments(1))
tag    = WScript.Arguments(2)

Dim opc
Set opc = CreateObject("OPC.Automation.1")
If Err.Number <> 0 Then
    WScript.Echo "ERROR:OPC.Automation.1 not found"
    WScript.Quit 1
End If

opc.Connect progId
If Err.Number <> 0 Then
    WScript.Echo "ERROR:Connect failed - " & Err.Description
    WScript.Quit 1
End If

Dim grp, item
Set grp = opc.OPCGroups.Add("CMD")
grp.IsActive = True
Set item = grp.OPCItems.AddItem(tag, 1)
If Err.Number <> 0 Then
    WScript.Echo "ERROR:AddItem failed - " & Err.Description
    opc.Disconnect
    WScript.Quit 1
End If

If action = "WRITE" Then
    Dim writeVal
    writeVal = CInt(WScript.Arguments(3))
    item.Write writeVal
    If Err.Number = 0 Then
        WScript.Echo "OK"
    Else
        WScript.Echo "ERROR:" & Err.Description
    End If

ElseIf action = "READ" Then
    Dim val, qual, ts
    item.Read 1, val, qual, ts
    If Err.Number = 0 Then
        WScript.Echo "OK:" & val
    Else
        WScript.Echo "ERROR:" & Err.Description
    End If
End If

opc.OPCGroups.RemoveAll
opc.Disconnect
