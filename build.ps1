# Builds Humbug NX with Docker: the controller self-test, the 32-bit game program and
# the launcher .nro. No game files go into the .nro.
#   powershell -ExecutionPolicy Bypass -File .\build.ps1
$ErrorActionPreference = 'Stop'
$here = (Resolve-Path $PSScriptRoot).Path
$port = Join-Path $here 'port'
$a64 = 'devkitpro/devkita64:latest'

docker run --rm -v "${port}:/w" -w /w $a64 bash -c 'gcc -std=gnu11 -Wall -Isource test/selftest.c source/hb_controls.c -lm -o /tmp/selftest && /tmp/selftest'
if ($LASTEXITCODE -ne 0) { throw 'controller self-test failed' }

& powershell -ExecutionPolicy Bypass -File (Join-Path $port 'build.ps1')
if ($LASTEXITCODE -ne 0) { throw '32-bit build failed' }

New-Item -ItemType Directory -Force (Join-Path $port 'launcher\romfs') | Out-Null
$saved = $ErrorActionPreference
$ErrorActionPreference = 'Continue'
docker run --rm --platform linux/amd64 -v "${port}:/work" -w /work/launcher $a64 bash -lc 'make -j$(nproc) A32=/work/runtime' 2>&1 | ForEach-Object { "$_" }
$code = $LASTEXITCODE
$ErrorActionPreference = $saved
if ($code -ne 0) { throw 'launcher build failed' }

$dist = Join-Path $here 'dist\switch\humbug_nx'
New-Item -ItemType Directory -Force $dist | Out-Null
Copy-Item (Join-Path $port 'launcher\humbug_nx.nro') $dist -Force
Get-Item (Join-Path $dist 'humbug_nx.nro') | Select-Object Name, Length
