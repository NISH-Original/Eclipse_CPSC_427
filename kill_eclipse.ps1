# Kill any running eclipse.exe processes and unlock files
# Run this script if you get LNK1168 errors when building

Write-Host "Checking for running eclipse.exe processes..."

$processes = Get-Process -Name eclipse -ErrorAction SilentlyContinue

if ($processes) {
    Write-Host "Found $($processes.Count) eclipse.exe process(es). Killing them..."
    $processes | Stop-Process -Force -ErrorAction SilentlyContinue
    Start-Sleep -Milliseconds 500
    
    # Check again and kill any remaining
    $remaining = Get-Process -Name eclipse -ErrorAction SilentlyContinue
    if ($remaining) {
        Write-Host "Some processes still running, force killing again..."
        $remaining | Stop-Process -Force -ErrorAction SilentlyContinue
        Start-Sleep -Milliseconds 300
    }
    
    Write-Host "Done! You can now build your project."
} else {
    Write-Host "No eclipse.exe processes found."
}

# Try to unlock the executable file if it exists
$exePath = "build\Debug\eclipse.exe"
if (Test-Path $exePath) {
    Write-Host "Attempting to unlock $exePath..."
    try {
        # Try to remove read-only attribute if set
        $file = Get-Item $exePath -ErrorAction SilentlyContinue
        if ($file) {
            $file.IsReadOnly = $false
        }
        Write-Host "File unlocked."
    } catch {
        Write-Host "Could not unlock file (this is usually okay): $_"
    }
}

