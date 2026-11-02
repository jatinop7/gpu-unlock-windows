[CmdletBinding()]
param(
    [Parameter()]
    [string] $OutFile
)

$ErrorActionPreference = 'Stop'

function Get-RegistrySnapshot {
    param(
        [Parameter(Mandatory)] [string] $Path,
        [Parameter(Mandatory)] [string[]] $Names
    )

    $result = [ordered]@{}
    try {
        $item = Get-ItemProperty -LiteralPath $Path -ErrorAction Stop
        foreach ($name in $Names) {
            $property = $item.PSObject.Properties[$name]
            $result[$name] = if ($null -ne $property) { [string]$property.Value } else { $null }
        }
    } catch {
        foreach ($name in $Names) { $result[$name] = $null }
    }
    return $result
}

function ConvertTo-PciIdentity {
    param([Parameter(Mandatory)] [string] $InstanceId)

    $pattern = '(?i)^PCI\\VEN_(?<vendor>[0-9a-f]{4})&DEV_(?<device>[0-9a-f]{4})(?:&SUBSYS_(?<subsystem>[0-9a-f]{8}))?(?:&REV_(?<revision>[0-9a-f]{2}))?'
    $match = [regex]::Match($InstanceId, $pattern)
    if (-not $match.Success) { return $null }

    $subsystem = $match.Groups['subsystem'].Value
    return [ordered]@{
        vendor_id = $match.Groups['vendor'].Value.ToLowerInvariant()
        device_id = $match.Groups['device'].Value.ToLowerInvariant()
        subsystem_device_id = if ($subsystem) { $subsystem.Substring(0, 4).ToLowerInvariant() } else { $null }
        subsystem_vendor_id = if ($subsystem) { $subsystem.Substring(4, 4).ToLowerInvariant() } else { $null }
        revision = if ($match.Groups['revision'].Success) { $match.Groups['revision'].Value.ToLowerInvariant() } else { $null }
    }
}

function Get-DisplayAdapters {
    $pnputil = Join-Path $env:SystemRoot 'System32\pnputil.exe'
    if (-not (Test-Path -LiteralPath $pnputil)) { return @() }

    try {
        $lines = & $pnputil /enum-devices /class Display /connected /properties 2>$null
        if ($LASTEXITCODE -ne 0) { return @() }
        $content = $lines -join "`n"
    } catch {
        return @()
    }

    $starts = [regex]::Matches(
        $content,
        '(?im)^\s*DEVPKEY_Device_DeviceDesc \[String\]:\s*$'
    )
    $adapters = @()
    for ($i = 0; $i -lt $starts.Count; $i++) {
        $start = $starts[$i].Index
        $end = if ($i + 1 -lt $starts.Count) { $starts[$i + 1].Index } else { $content.Length }
        $block = $content.Substring($start, $end - $start)

        $nameMatch = [regex]::Match($block, '(?im)^\s*DEVPKEY_Device_DeviceDesc \[String\]:\s*\r?\n\s*(?<value>[^\r\n]+)')
        $idMatch = [regex]::Match($block, '(?im)^\s*DEVPKEY_Device_InstanceId \[String\]:\s*\r?\n\s*(?<value>[^\r\n]+)')
        if (-not $idMatch.Success) { continue }

        $driverMatch = [regex]::Match($block, '(?im)^\s*DEVPKEY_Device_DriverVersion \[String\]:\s*\r?\n\s*(?<value>[^\r\n]+)')
        $instanceId = $idMatch.Groups['value'].Value.Trim()
        $pci = ConvertTo-PciIdentity -InstanceId $instanceId
        $adapters += [ordered]@{
            name = if ($nameMatch.Success) { $nameMatch.Groups['value'].Value.Trim() } else { $null }
            pci = $pci
            pci_status = if ($null -ne $pci) { 'parsed' } else { 'unrecognized-or-non-pci' }
            driver_version = if ($driverMatch.Success) { $driverMatch.Groups['value'].Value.Trim() } else { $null }
        }
    }
    return $adapters
}

function Get-NvidiaSmiInventory {
    $command = Get-Command nvidia-smi.exe -ErrorAction SilentlyContinue
    if ($null -eq $command) { return @() }

    $fields = @('name', 'pci.device_id', 'pci.sub_device_id', 'driver_version', 'vbios_version', 'memory.total', 'compute_cap')
    try {
        $lines = & $command.Source "--query-gpu=$($fields -join ',')" --format=csv,noheader 2>$null
        if ($LASTEXITCODE -ne 0) { return @() }
    } catch {
        return @()
    }

    $records = @()
    foreach ($line in $lines) {
        $values = ([string]$line).Split(',') | ForEach-Object { $_.Trim() }
        if ($values.Count -lt $fields.Count) { continue }
        $records += [ordered]@{
            name = $values[0]
            device_id_raw = $values[1]
            subsystem_id_raw = $values[2]
            driver_version = $values[3]
            vbios_version = $values[4]
            memory_total = $values[5]
            compute_capability = $values[6]
        }
    }
    return $records
}

function Get-CimDisplayAdapters {
    try {
        $devices = @(Get-CimInstance -ClassName Win32_VideoController -Property Name, PNPDeviceID, DriverVersion -ErrorAction Stop)
        return @($devices | ForEach-Object {
            $pci = if ($_.PNPDeviceID) { ConvertTo-PciIdentity -InstanceId ([string]$_.PNPDeviceID) } else { $null }
            [ordered]@{
                name = [string]$_.Name
                pci = $pci
                pci_status = if ($null -ne $pci) { 'parsed' } elseif ($_.PNPDeviceID) { 'unrecognized-or-non-pci' } else { 'unavailable' }
                driver_version = [string]$_.DriverVersion
            }
        })
    } catch {
        return @()
    }
}

$os = Get-RegistrySnapshot -Path 'HKLM:\SOFTWARE\Microsoft\Windows NT\CurrentVersion' -Names @(
    'ProductName', 'EditionID', 'DisplayVersion', 'CurrentBuild', 'UBR', 'InstallationType'
)
$system = Get-RegistrySnapshot -Path 'HKLM:\HARDWARE\DESCRIPTION\System\BIOS' -Names @(
    'SystemManufacturer', 'SystemProductName', 'SystemFamily',
    'BaseBoardManufacturer', 'BaseBoardProduct', 'BIOSVendor', 'BIOSVersion', 'BIOSReleaseDate'
)

$cpuNames = @()
try {
    $cpuPath = 'HKLM:\HARDWARE\DESCRIPTION\System\CentralProcessor'
    $cpuNames = @(Get-ChildItem -LiteralPath $cpuPath -ErrorAction Stop | ForEach-Object {
        $value = (Get-ItemProperty -LiteralPath $_.PSPath -Name ProcessorNameString -ErrorAction Stop).ProcessorNameString
        if ($value) { ([string]$value).Trim() }
    } | Sort-Object -Unique)
} catch {
    $cpuNames = @()
}

$adapters = @(Get-DisplayAdapters)
if ($adapters.Count -eq 0) { $adapters = @(Get-CimDisplayAdapters) }
$nvidiaSmi = @(Get-NvidiaSmiInventory)
$missing = @()
if (-not $os.ProductName -or -not $os.CurrentBuild) { $missing += 'os' }
if (-not $system.SystemProductName -or -not $system.BIOSVersion) { $missing += 'system-or-bios' }
if ($cpuNames.Count -eq 0) { $missing += 'cpu' }
if ($adapters.Count -eq 0) { $missing += 'display-adapters' }

$report = [ordered]@{
    schema_version = 1
    purpose = 'gpu-unlock-windows-compatibility-inventory'
    collector_version = '0.1.0'
    collection_complete = ($missing.Count -eq 0)
    missing_sections = @($missing | Sort-Object -Unique)
    hardware = [ordered]@{
        os = $os
        system = $system
        cpu_names = $cpuNames
        display_adapters = $adapters
        nvidia_smi = $nvidiaSmi
        architecture = if ([Environment]::Is64BitOperatingSystem) { 'x64' } else { 'x86' }
    }
    assessment = [ordered]@{
        status = 'not-assessed'
        can_unlock = $false
        reason = 'Windows inventory cannot validate pre-boot state or transaction safety.'
    }
    not_collected = @(
        'hostname, username, serial numbers, UUIDs, and raw PnP instance suffixes'
        'Secure Boot state, UEFI display ownership, and complete PCI ancestry'
        'PCI BAR layout, MMIO registers, PROM/VBIOS bytes, and firmware captures'
        'GPU cold state, power-gating behavior, DMA ownership, and cleanup evidence'
    )
}

$json = ConvertTo-Json -InputObject $report -Depth 8
if ($OutFile) {
    $fullPath = [System.IO.Path]::GetFullPath($OutFile)
    [System.IO.File]::WriteAllText($fullPath, $json + [Environment]::NewLine, [System.Text.UTF8Encoding]::new($false))
} else {
    $json
}
