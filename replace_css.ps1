$bytes = [System.IO.File]::ReadAllBytes('C:\Users\Diman\Documents\PlatformIO\Projects\relay_ota\src\html_page.cpp')
$text = [System.Text.Encoding]::UTF8.GetString($bytes)
$dash = [char]0x2500
$startMarker = '/* ' + $dash + $dash + ' Sensor Cards (Simple) ' + $dash + $dash + ' */'
$endMarker = '.sensor-age{font-size:10px;color:var(--text2);margin-top:2px}'
$startIdx = $text.IndexOf($startMarker)
$endIdx = $text.IndexOf($endMarker, $startIdx)
$blockEnd = $endIdx + $endMarker.Length

$newCSSPath = 'C:\Users\Diman\Documents\PlatformIO\Projects\relay_ota\new_css.txt'
$newCSS = [System.IO.File]::ReadAllText($newCSSPath, [System.Text.Encoding]::UTF8)

$text = $text.Remove($startIdx, $blockEnd - $startIdx)
$text = $text.Insert($startIdx, $newCSS)

$enc = [System.Text.Encoding]::UTF8
[System.IO.File]::WriteAllBytes('C:\Users\Diman\Documents\PlatformIO\Projects\relay_ota\src\html_page.cpp', $enc.GetBytes($text))
Write-Host 'CSS replaced successfully'
