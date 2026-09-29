<#
.SYNOPSIS
    从 Api.h 生成 VInput.py 的 ctypes 封装。
.DESCRIPTION
    解析 Api.h 中的 VK_* 常量（含行尾注释）、DLLAPI 函数声明及 XML 注释
    （summary / param / returns），从 Version.h 读取版本号，
    生成带 docstring 的 Python ctypes 封装。
.EXAMPLE
    .\gen_vinput_py.ps1 -ApiH .\VInput\Api.h -OutPy .\x64dll\VInput.py -VersionHeader .\VInput\Version.h
#>
param(
    [Parameter(Mandatory=$true)][string]$ApiH,
    [Parameter(Mandatory=$true)][string]$OutPy,
    [string]$VersionHeader = ""
)

$ErrorActionPreference = "Stop"

# ---------- C 类型到 ctypes 的映射 ----------
$CTypeMap = @{
    "BOOL"        = "c_bool"
    "INT32"       = "c_int"
    "INT8"        = "c_byte"
    "UINT8"       = "c_byte"
    "UTF8_STRING" = "c_char_p"
    "void"        = $null
}

# ---------- 读取源文件 ----------
if (-not (Test-Path $ApiH)) {
    Write-Error "Api.h not found: $ApiH"
    exit 1
}
$text = Get-Content -Path $ApiH -Raw -Encoding UTF8

# ---------- 读取版本号 ----------
$version = "unknown"
if ($VersionHeader -and (Test-Path $VersionHeader)) {
    $verText = Get-Content -Path $VersionHeader -Raw -Encoding UTF8
    if ($verText -match '#define\s+VERSION_STRING\s+"([^"]+)"') {
        $version = $Matches[1]
    }
}

# ---------- 解析 #define VK_* （含行尾注释） ----------
$defines = @()
$defineRegex = [regex]::new(
    '^\s*#define\s+(?<name>VK_\w+)\s+(?<value>0x[0-9A-Fa-f]+|\d+)\s*(?:(?://|/\*)\s*(?<comment>.*?)(?:\s*\*/)?)?\s*$',
    'Multiline')

foreach ($m in $defineRegex.Matches($text)) {
    $comment = ""
    if ($m.Groups['comment'].Success) {
        $comment = $m.Groups['comment'].Value.Trim()
        $comment = $comment -replace '\s+', ' '
    }

    $defines += [PSCustomObject]@{
        Name    = $m.Groups['name'].Value
        Value   = $m.Groups['value'].Value
        Comment = $comment
    }
}

# ---------- 逐行扫描，提取函数、summary、param、returns ----------
$allLines = $text -split "\r\n|\r|\n"

$funcs = @()
$summaryLines = New-Object System.Collections.ArrayList
$inSummary = $false
$pendingParams = @{}       # Hashtable，支持 Clone
$pendingReturns = ""

$declRegex = [regex]'DLLAPI\s+(?<ret>\w+)\s+STDCALL\s+(?<name>\w+)\s*\((?<args>[^)]*)\)\s*;'

foreach ($line in $allLines) {
    # 1. <summary> 开始
    if ($line -match '^\s*///\s*<summary>') {
        $inSummary = $true
        $summaryLines.Clear() | Out-Null
        continue
    }

    # 2. </summary> 结束
    if ($line -match '^\s*///\s*</summary>') {
        $inSummary = $false
        continue
    }

    # 3. summary 内部内容
    if ($inSummary -and $line -match '^\s*///') {
        $content = ($line -replace '^\s*///\s?', '').TrimEnd()
        [void]$summaryLines.Add($content)
        continue
    }

    # 4. <param name="xxx">desc</param>
    if ($line -match '^\s*///\s*<param\s+name="(?<pname>\w+)">(?<pdesc>.*?)</param>') {
        $pendingParams[$Matches['pname']] = $Matches['pdesc'].Trim()
        continue
    }

    # 5. <returns>desc</returns>
    if ($line -match '^\s*///\s*<returns>(?<rdesc>.*?)</returns>') {
        $pendingReturns = $Matches['rdesc'].Trim()
        continue
    }

    # 6. 其他 /// 行忽略
    if ($line -match '^\s*///') {
        continue
    }

    # 7. 检查函数声明
    $m = $declRegex.Match($line)
    if ($m.Success) {
        $retType = $m.Groups['ret'].Value
        $name    = $m.Groups['name'].Value
        $argsStr = $m.Groups['args'].Value.Trim()

        $args = @()
        if ($argsStr -and $argsStr -ne 'void') {
            foreach ($arg in ($argsStr -split ',')) {
                $arg = $arg.Trim()
                if ($arg -match '^(\w+)\s+(\w+)$') {
                    $args += [PSCustomObject]@{
                        CType = $Matches[1]
                        Name  = $Matches[2]
                    }
                }
            }
        }

        # summary 文本
        $summaryText = ""
        if ($summaryLines.Count -gt 0) {
            $tmp = @($summaryLines | Where-Object { $_ -ne '' })
            if ($tmp.Count -gt 0) {
                $summaryText = ($tmp -join "`n")
            }
        }

        $funcs += [PSCustomObject]@{
            RetType = $retType
            Name    = $name
            Args    = $args
            Summary = $summaryText
            Params  = $pendingParams.Clone()
            Returns = $pendingReturns
        }

        # 清空，为下一个函数准备
        $summaryLines.Clear() | Out-Null
        $inSummary = $false
        $pendingParams = @{}
        $pendingReturns = ""
        continue
    }

    # 8. 其他非注释行，清空 pending
    if ($line -notmatch '^\s*$') {
        if ($summaryLines.Count -gt 0) {
            $summaryLines.Clear() | Out-Null
        }
        if ($pendingParams.Count -gt 0) {
            $pendingParams = @{}
        }
        if ($pendingReturns) {
            $pendingReturns = ""
        }
    }
}

# ---------- 工具函数 ----------
function ConvertTo-SnakeCase([string]$name) {
    $s1 = [regex]::Replace($name, '(.)([A-Z][a-z]+)', '$1_$2')
    return [regex]::Replace($s1, '([a-z0-9])([A-Z])', '$1_$2').ToLower()
}

function ConvertTo-CtypesArg($cType, [string]$argName) {
    switch ($cType) {
        'BOOL'        { return "c_bool($argName)" }
        'INT32'       { return "c_int($argName)" }
        'INT8'        { return "c_byte($argName)" }
        'UINT8'       { return "c_byte($argName)" }
        'UTF8_STRING' {
            return "$argName.encode('utf-8') if isinstance($argName, str) else $argName"
        }
        default { return $argName }
    }
}

# ---------- 生成输出 ----------
$sb = New-Object System.Text.StringBuilder
function Add-Line([string]$s = '') { [void]$sb.AppendLine($s) }

$srcName = Split-Path $ApiH -Leaf
$now = Get-Date -Format "yyyy-MM-dd HH:mm:ss"

# ===== 文件头 =====
Add-Line "# ============================================================================="
Add-Line "# VInput.py - Python wrapper for VInput.dll"
Add-Line "#"
Add-Line "# Version    : $version"
Add-Line "# Source     : $srcName"
Add-Line "# Generated  : $now"
Add-Line "# Generator  : gen_vinput_py.ps1"
Add-Line "# Platform   : Windows x64 only"
Add-Line "#"
Add-Line "# This file is auto-generated. Do not edit manually."
Add-Line "# Any changes will be overwritten on the next build."
Add-Line "# ============================================================================="
Add-Line ''
Add-Line 'import ctypes'
Add-Line 'from ctypes import c_bool, c_int, c_byte, c_char_p, c_void_p'
Add-Line 'import os'
Add-Line ''
Add-Line "__version__ = `"$version`""
Add-Line ''

# ===== 常量 =====
Add-Line '# ============================================================================='
Add-Line '# Virtual Key constants'
Add-Line '# ============================================================================='

$mouseKeys = $defines | Where-Object { $_.Name -match 'VK_(L|R|M|X)BUTTON' }
$otherKeys = $defines | Where-Object { $_.Name -notmatch 'VK_(L|R|M|X)BUTTON' }

function Add-Constant([PSCustomObject]$d) {
    if ($d.Comment) {
        Add-Line ("{0,-20}= {1,-8}# {2}" -f $d.Name, $d.Value, $d.Comment)
    } else {
        Add-Line ("{0,-20}= {1}" -f $d.Name, $d.Value)
    }
}

Add-Line ''
Add-Line '# Mouse buttons'
foreach ($d in $mouseKeys) { Add-Constant $d }

Add-Line ''
Add-Line '# Keyboard keys'
foreach ($d in $otherKeys) { Add-Constant $d }
Add-Line ''

# ===== DLL 加载 =====
Add-Line '# ============================================================================='
Add-Line '# DLL loading'
Add-Line '# ============================================================================='
Add-Line '_dll = None'
Add-Line '_funcs_declared = False'
Add-Line ''
Add-Line 'def _load_dll(dll_path=None):'
Add-Line '    """Load VInput.dll from script directory or given path."""'
Add-Line '    global _dll'
Add-Line '    if _dll is not None:'
Add-Line '        return _dll'
Add-Line ''
Add-Line '    if dll_path is None:'
Add-Line '        base_dir = os.path.dirname(os.path.abspath(__file__))'
Add-Line '        dll_path = os.path.join(base_dir, "VInput.dll")'
Add-Line ''
Add-Line '    if not os.path.exists(dll_path):'
Add-Line '        raise OSError(f"DLL not found: {dll_path}")'
Add-Line ''
Add-Line '    _dll = ctypes.WinDLL(dll_path)'
Add-Line '    return _dll'
Add-Line ''

# ===== 函数声明 =====
Add-Line '# ============================================================================='
Add-Line '# Function signatures (argtypes / restype)'
Add-Line '# ============================================================================='
Add-Line 'def _declare_funcs(dll):'
Add-Line '    """Set argtypes and restype for all exported functions."""'
Add-Line '    global _funcs_declared'
Add-Line '    if _funcs_declared:'
Add-Line '        return'
Add-Line ''
foreach ($f in $funcs) {
    $argTypes = @()
    foreach ($a in $f.Args) {
        $ct = $CTypeMap[$a.CType]
        if (-not $ct) { $ct = 'c_void_p' }
        $argTypes += $ct
    }
    $argTypesStr = ($argTypes -join ', ')

    Add-Line "    dll.$($f.Name).argtypes = [$argTypesStr]"

    if ($f.RetType -eq 'void') {
        Add-Line "    dll.$($f.Name).restype = None"
    } else {
        $rt = $CTypeMap[$f.RetType]
        if (-not $rt) { $rt = 'c_void_p' }
        Add-Line "    dll.$($f.Name).restype = $rt"
    }
}
Add-Line '    _funcs_declared = True'
Add-Line ''

# ===== 公共 API =====
Add-Line '# ============================================================================='
Add-Line '# Public API'
Add-Line '# ============================================================================='

foreach ($f in $funcs) {
    $pyName = ConvertTo-SnakeCase $f.Name
    $paramList = ($f.Args | ForEach-Object { $_.Name }) -join ', '

    $cArgs = ($f.Args | ForEach-Object { "$($_.CType) $($_.Name)" }) -join ', '
    if (-not $cArgs) { $cArgs = 'void' }
    $cSig = "DLLAPI $($f.RetType) STDCALL $($f.Name)($cArgs);"

    Add-Line ''
    Add-Line "# $cSig"
    Add-Line "def ${pyName}($paramList):"

    $hasSummary = [bool]$f.Summary
    $hasParams  = ($f.Params.Count -gt 0)
    $hasReturns = [bool]$f.Returns

    if ($hasSummary -or $hasParams -or $hasReturns) {
        Add-Line '    """'
        if ($hasSummary) {
            foreach ($sl in ($f.Summary -split "`n")) {
                Add-Line "    $sl"
            }
        }

        if ($hasParams) {
            Add-Line ''
            Add-Line '    Args:'
            foreach ($a in $f.Args) {
                if ($f.Params.ContainsKey($a.Name)) {
                    Add-Line "        $($a.Name): $($f.Params[$a.Name])"
                } else {
                    Add-Line "        $($a.Name):"
                }
            }
        }

        if ($hasReturns) {
            Add-Line ''
            Add-Line '    Returns:'
            Add-Line "        $($f.Returns)"
        }

        Add-Line '    """'
    } else {
        Add-Line ('    """' + $f.Name + '"""')
    }

    Add-Line '    dll = _load_dll()'
    Add-Line '    _declare_funcs(dll)'

    $callArgs = @()
    foreach ($a in $f.Args) {
        $callArgs += ConvertTo-CtypesArg $a.CType $a.Name
    }
    $callArgsStr = ($callArgs -join ', ')

    if ($f.RetType -eq 'void') {
        Add-Line "    dll.$($f.Name)($callArgsStr)"
    } else {
        Add-Line "    return dll.$($f.Name)($callArgsStr)"
    }
}

# ---------- 写出文件（UTF-8 无 BOM） ----------
$outDir = Split-Path $OutPy -Parent
if ($outDir -and -not (Test-Path $outDir)) {
    New-Item -ItemType Directory -Path $outDir | Out-Null
}

$utf8NoBom = New-Object System.Text.UTF8Encoding($false)
[System.IO.File]::WriteAllText($OutPy, $sb.ToString(), $utf8NoBom)

Write-Host "Generated $OutPy"
Write-Host "  Version   : $version"
Write-Host "  Constants : $($defines.Count)"
Write-Host "  Functions : $($funcs.Count)"