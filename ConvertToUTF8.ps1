# convert all source files in a folder to utf8

param (
    [Parameter(HelpMessage="Show what would happen")]
    [Alias("what-if")]
    [switch] $whatif = $false,

    [Parameter(HelpMessage="Descend into subdirectories")]
    [switch] $recurse = $false,

    [Parameter(HelpMessage="Comma separated list of file wildcards to include")]
    [string] $include,

    [Parameter(HelpMessage="Comma separated list of file wildcards to exclude")]
    [string] $exclude,

    [Parameter(HelpMessage="Encoding to change the files to (unicode|bigendianunicode|utf8|utf7|utf32|ascii|default|oem)]")]
    [string] $encoding = "utf8"
)

$whatifparam = ""
$recurseparam = ""
if($whatif) {
    $whatifparam = "-what-if"
}
if($recurse) {
    $recurseparam = "-recurse"
}

Write-Host "Get-ChildItem $recurseparam -include $include -exclude $exclude"

$modified = 0
(Get-ChildItem $recurseparam -include ${$include} -exclude ${$exclude}) | ForEach-Object {
    Write-Host "Found  $_"
    $modified += 1
    (Get-Content $_) | Out-File -encoding $encoding $whatifparam $_
}

Write-Host "Modified $modified files"
