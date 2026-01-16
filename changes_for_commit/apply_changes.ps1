# Run this script from inside the repo root (where you copied `changes_for_commit`).
# It will copy files from `./changes_for_commit` into the parent repo root, overwriting existing files.

$src = Join-Path (Get-Location) 'changes_for_commit'
$dst = Get-Location

Write-Host "Copying files from $src to $dst (this will overwrite existing files)"

Get-ChildItem -Path $src -Recurse | ForEach-Object {
    $rel = $_.FullName.Substring($src.Length).TrimStart('\')
    $target = Join-Path $dst $rel

    if ($_.PSIsContainer) {
        if (!(Test-Path $target)) { New-Item -ItemType Directory -Path $target | Out-Null }
    } else {
        Copy-Item -Path $_.FullName -Destination $target -Force
        Write-Host "Copied: $rel"
    }
}

Write-Host "Done. Verify changes and run: git add . ; git commit -m 'Apply changes from changes_for_commit'"
