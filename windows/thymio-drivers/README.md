# Thymio Drivers

## WIn 8 and greater

 - `inf2cat /driver:. /os:8_X64,8_X86,Server8_X64,Server2008R2_X64,10_X86,10_X64,Server10_X64`
 - `signtool sign /tr http://timestamp.digicert.com /td sha256 /fd sha256 /du "http://http://www.mobsya.org/" /n "Association Mobsya" mchpcdc.cat`
 - `signtool sign /tr http://timestamp.digicert.com /td sha256 /fd sha256 /du "http://http://www.mobsya.org/" /n "Association Mobsya" mchpcdcw.cat`

## Win 7

 - `inf2cat /driver:. /os:Server2008R2_X64,7_X64,7_X86,Server2008_X64,Server2008_X86,Vista_X64,Vista_X86,Server2003_X64,Server2003_X86`
 - `signtool sign /tr http://timestamp.digicert.com /td sha256 /fd sha256 /du "http://http://www.mobsya.org/" /n "Association Mobsya" mchpcdcw.cat`
 - `signtool sign /tr http://timestamp.digicert.com /td sha256 /fd sha256 /du "http://http://www.mobsya.org/" /n "Association Mobsya" mchpcdc.cat`
