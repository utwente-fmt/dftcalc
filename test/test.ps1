Function All-Match ($AllArgs, $Needed) {
    $Positive = $Needed | Select-String -AllMatches '[+]"[^"]+"';
    $Negative = $Needed | Select-String -AllMatches '[!]"[^"]+"';
    $Ret = $true
    $Positive | ForEach-Object {
        $Pattern = $_.Matches.Value.Substring(2, $_.Matches.Value.Length - 3);
        $Match = $AllArgs | Select-String -Quiet -SimpleMatch -Pattern "$Pattern";
        if (!$Match) {
            $Ret = $false;
        }
    }
    $Negative | ForEach-Object {
        $Pattern = $_.Matches.Value.Substring(2, $_.Matches.Value.Length - 3);
        $Match = $AllArgs | Select-String -Quiet -SimpleMatch -Pattern "$Pattern";
        if ($Match) {
            $Ret = $false;
        }
    }
    return $Ret;
}

Function Should-Fail ($Dft, $AllArgs) {
    $Ret = $false;
    Select-String -Pattern '(\S+)\s+(.*)' -Path 'expect-fail.txt' | ForEach-Object {
        $File = $_.Matches.Groups[1].Value;
        if ($File = $Dft) {
            $Condition = $_.Matches.Groups[2].Value;
            if (All-Match "$AllArgs" "$Condition") {
                $Ret = $true;
            }
        }
    }
    return $Ret;
}

function Split-Interval ($Interval) {
    $Split = $Interval | Select-String "(.*)\[(.*);\s*(.*)\](.*)";
    if ($Split.Matches.Count -eq 0) {
        $Value = [System.Decimal]($Interval);
        return $Value,$Value
    }
    $Prefix = $Split.Matches.Groups[1].Value;
    $Min = $Split.Matches.Groups[2].Value;
    $Max = $Split.Matches.Groups[3].Value;
    $Exponent = $Split.Matches.Groups[4].Value;
    $Min="$Prefix$Min$Exponent";
    $Max="$Prefix$Max$Exponent";
    return [System.Decimal]($Min), [System.Decimal]($Max);
}

function Compare-Result ($Computed, $Expected) {
    $AsFraction = $Expected | Select-String "(.*)/(.*)";
    if ($AsFraction.Matches.Count -eq 1) {
        $Numerator = [System.Decimal]($AsFraction.Matches[0].Groups[1].Value);
        $Denominator = [System.Decimal]($AsFraction.Matches[0].Groups[2].Value);
        $ExpectedLow = $Numerator / $Denominator;
        $ExpectedHigh = $ExpectedLow;
    } else {
        $ExpectedLow, $ExpectedHigh = Split-Interval "$Expected"
    }

    $ComputedLow,$ComputedHigh = Split-Interval $Computed;

    if (($ComputedLow -ge $ExpectedLow) -and ($ComputedHigh -le $ExpectedHigh)) {
        return "Pass",0,"Exact: Got $Computed, equals/inside expected $Expected";
    } elseif (($ComputedLow -le $ExpectedLow) -and ($ComputedHigh -ge $ExpectedHigh)) {
        return "Pass",0,"Overlap: Expected $Expected, got $Computed";
    } else {
        return "Fail",1,"Expected $Expected, got $Computed";
    }
}

function Run-Test {
    param ($File, $TestArgs, $Result)
    $AllArgs = "$TestArgs -p $File"
    $ShouldFail = Should-Fail "$File" "$AllArgs"

    try  {
        Start-Process -FilePath dftcalc -ArgumentList $AllArgs.Split(" ") -NoNewWindow -Wait -RedirectStandardOutput "$File.output.txt" -RedirectStandardError "$File.error.txt"
    } catch {
        Write-Information "FAIL: $File $TestArgs (Failed to start)";
        Write-Information $_
        return 1;
    }
    $Results = Select-String -Path "$File.output.txt" ".*=(.*)";
    if ($Results.Matches.Count -eq 0) {
        if ($ShouldFail) {
            Write-Information "Pass: $File $Testargs (Failed as expected)"
            return 0;
        }
        Write-Information "FAIL: $File $TestArgs (No output)";
        return 1;
    } elseif ($Results.Matches.Count -ne 1) {
        Write-Information "FAIL: $File $TestArgs (Multiple outputs)";
        return 1;
    }
    $Computed = $Results.Matches[0].Groups[1].Value;
    if ($ShouldFail) {
        Write-Information "FAIL: $File $TestArgs (Got $Computed, expected failure)";
        return 1;
    }
    $Judgement = "";
    $Judgement, $Failed, $Comment = Compare-Result $Computed $Result;
    Write-Information "${Judgement}: $File $TestArgs ($Comment)";
    return $Failed;
}

$InformationPreference = "Continue";
$GlobalArgs = $args;
if ($GlobalArgs.Length -eq 0) {
    $GlobalArgs = "--imrmc";
}
$FailCount = 0;
$TestCount = 0;
Select-String -Path tests.txt "(\S+)\s+""(.*)""\s+(.+)" | ForEach-Object {
    $FileName = $_.Matches[0].Groups[1];
    $Arguments = $_.Matches[0].Groups[2];
    $Result = $_.Matches[0].Groups[3].Value;
    $TestCount += 1;
    $Result = Run-Test $FileName.Value "$Arguments $GlobalArgs" $Result;
    $FailCount += $Result;
}

if ($FailCount -eq 0) {
    Write-Information "$TestCount tests executed, all passed";
} else {
    Throw "$TestCount tests executed, $FailCount failed";
}
