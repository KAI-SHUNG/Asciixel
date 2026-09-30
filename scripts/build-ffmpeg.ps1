param([int]$Jobs = 4)

$ErrorActionPreference = 'Stop'

$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$source = Join-Path $root 'third_party/ffmpeg'
if (-not (Test-Path (Join-Path $source 'configure'))) {
    throw 'FFmpeg submodule is missing. Run git submodule update --init --recursive.'
}

$gitRoot = Split-Path (Split-Path (Get-Command git).Source -Parent) -Parent
$bash = Join-Path $gitRoot 'bin/bash.exe'
if (-not (Test-Path $bash)) {
    throw 'Git for Windows Bash is required to build FFmpeg.'
}

$mingwRoot = Split-Path (Split-Path (Get-Command gcc).Source -Parent) -Parent
$build = Join-Path $root 'build/ffmpeg'
$install = Join-Path $root 'build/ffmpeg-install'
New-Item -ItemType Directory -Force $build | Out-Null

function To-PosixPath($path) {
    $converted = & $bash -lc "cygpath -u '$path'"
    if ($LASTEXITCODE -ne 0) { throw "Cannot convert path: $path" }
    return $converted.Trim()
}

$buildPath = To-PosixPath $build
$sourcePath = To-PosixPath $source
$installPath = To-PosixPath $install
$mingwPath = To-PosixPath $mingwRoot

$configure = "'$sourcePath/configure' --prefix='$installPath' " +
    '--disable-autodetect --disable-everything --enable-shared --disable-static ' +
    '--disable-programs --disable-doc --disable-x86asm --disable-network ' +
    '--enable-avformat --enable-avcodec --enable-swscale ' +
    '--enable-decoder=png,mjpeg,h264 --enable-encoder=png --enable-demuxer=image2,png_pipe,jpeg_pipe,mov,wav ' +
    '--enable-protocol=file --enable-parser=png,mjpeg,h264 --enable-zlib ' +
    "--extra-cflags=-I$mingwPath/include --extra-ldflags=-L$mingwPath/lib"

& $bash -lc "cd '$buildPath' && $configure"
if ($LASTEXITCODE -ne 0) { throw 'FFmpeg configuration failed.' }

# MinGW make.exe reads paths itself and needs Windows-style drive prefixes.
$rootPath = $root.Replace([IO.Path]::DirectorySeparatorChar, '/')
foreach ($file in @((Join-Path $build 'Makefile'), (Join-Path $build 'ffbuild/config.mak'))) {
    $contents = [IO.File]::ReadAllText($file)
    [IO.File]::WriteAllText($file, $contents.Replace((To-PosixPath $root), $rootPath))
}

& $bash -lc "cd '$buildPath' && make -j$Jobs"
if ($LASTEXITCODE -ne 0) { throw 'FFmpeg build failed.' }

foreach ($component in @('libavformat', 'libavcodec', 'libavutil', 'libswscale')) {
    $headers = Join-Path $install "include/$component"
    New-Item -ItemType Directory -Force $headers | Out-Null
    Copy-Item (Join-Path $source "$component/*.h") $headers
}
Copy-Item (Join-Path $build 'libavutil/avconfig.h') (Join-Path $install 'include/libavutil')
Copy-Item (Join-Path $build 'libavutil/ffversion.h') (Join-Path $install 'include/libavutil')

New-Item -ItemType Directory -Force (Join-Path $install 'lib'), (Join-Path $install 'bin') | Out-Null
foreach ($component in @('avformat', 'avcodec', 'avutil', 'swscale')) {
    Copy-Item (Join-Path $build "lib$component/$component.lib") (Join-Path $install 'lib')
    Copy-Item (Join-Path $build "lib$component/$component-*.dll") (Join-Path $install 'bin')
}
