param(
    [Parameter(Mandatory = $true)]
    [string]$Binary,

    [string]$OutputDirectory = "etw_runs",

    [string]$Arguments = "--stress-test --stress-summary",

    [switch]$SkipExporter
)

Set-StrictMode -Version Latest

if (-not (Test-Path $Binary)) {
    Write-Error "Binary '$Binary' does not exist. Build the game before profiling."
    exit 1
}

$timestamp = (Get-Date).ToUniversalTime().ToString("yyyyMMddTHHmmssZ")
$runDirectory = Join-Path $OutputDirectory $timestamp
New-Item -ItemType Directory -Path $runDirectory -Force | Out-Null

$etlPath = Join-Path $runDirectory "threading_hotspots.etl"
$summaryPath = Join-Path $runDirectory "threading_summary.csv"

Write-Host "Starting WPR CPU + ReferenceSet captures..."
wpr -start CPU -filemode | Out-Null
wpr -start ReferenceSet -filemode | Out-Null

try {
    Write-Host "Running stress harness: $Binary $Arguments"
    $process = Start-Process -FilePath $Binary -ArgumentList $Arguments -PassThru -Wait
    if ($process.ExitCode -ne 0) {
        Write-Warning "Stress harness exited with code $($process.ExitCode). Review logs for details."
    }
}
finally {
    Write-Host "Stopping WPR capture -> $etlPath"
    wpr -stop $etlPath | Out-Null
}

if (-not $SkipExporter) {
    $wpaProfile = Join-Path $PSScriptRoot "threading_hotspots.wpaProfile"
    if (Get-Command wpaexporter -ErrorAction SilentlyContinue) {
        if (Test-Path $wpaProfile) {
            Write-Host "Exporting WPA summary..."
            wpaexporter -i $etlPath -profile $wpaProfile -o $summaryPath | Out-Null
        }
        else {
            Write-Warning "Missing threading_hotspots.wpaProfile, skipping exporter output."
        }
    }
    else {
        Write-Warning "wpaexporter not found. Install Windows Performance Toolkit to enable automated summaries."
    }
}

Write-Host "ETW capture complete. Artifacts saved to $runDirectory"
