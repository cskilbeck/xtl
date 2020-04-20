rem see https://bitbucket.org/cskilbeck/vault/src/master/aws.txt
rem Access Key ID:          XXXX
rem Secret Access Key:      XXXX

rem `aws configure` to enter credentials
rem aws and node must be on path

node --check index.js
if errorlevel 1 goto done
del lambda.zip
"c:\Program Files\7-Zip\7z.exe" a -mmt8 -mx3 -tzip lambda.zip node_modules *.js
aws lambda update-function-code --function-name "L2_EU_WEST1" --zip-file "fileb://./lambda.zip" --publish 
:done
