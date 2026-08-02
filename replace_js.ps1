$bytes = [System.IO.File]::ReadAllBytes('C:\Users\Diman\Documents\PlatformIO\Projects\relay_ota\src\html_page.cpp')
$text = [System.Text.Encoding]::UTF8.GetString($bytes)

# Find start of JS block
$startMarker = '// '
$startMarker = $startMarker + [char]0x2500 + [char]0x2500 + ' '
$startMarker = $startMarker + [char]0x0644 + [char]0x06CC + [char]0x0633 + [char]0x062A
$startMarker = $startMarker + ' ' + [char]0x0633 + [char]0x0646 + [char]0x0633 + [char]0x0648 + [char]0x0631
$startMarker = $startMarker + [char]0x0647 + [char]0x0627

# Find the loadSensorsPage function and its end
$funcStart = $text.IndexOf('async function loadSensorsPage')
if ($funcStart -lt 0) {
    Write-Host 'ERROR: loadSensorsPage not found'
    exit 1
}

# Find the end: the next function after loadSensorsPage
$afterFunc = $text.IndexOf('function sOpenEdit', $funcStart)
if ($afterFunc -lt 0) {
    Write-Host 'ERROR: sOpenEdit not found'
    exit 1
}

# Read new JS
$newJS = [System.IO.File]::ReadAllText('C:\Users\Diman\Documents\PlatformIO\Projects\relay_ota\new_js.txt', [System.Text.Encoding]::UTF8)

# Replace: from "async function loadSensorsPage" up to (not including) "function sOpenEdit"
$text = $text.Remove($funcStart, $afterFunc - $funcStart)
$text = $text.Insert($funcStart, $newJS + "`n")

$enc = [System.Text.Encoding]::UTF8
[System.IO.File]::WriteAllBytes('C:\Users\Diman\Documents\PlatformIO\Projects\relay_ota\src\html_page.cpp', $enc.GetBytes($text))
Write-Host 'JS replaced successfully'
