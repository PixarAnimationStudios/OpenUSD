
The files ("schema.usda", "generatedSchema.usda" and
"plugInfo.json") in this directory are auto generated using 
usdgenschemafromsdr utility.

A schema.usda is populated using sdrNodes which are specified in a
json config. usdGenSchema is then run on this auto populated schema 
(with skipCodeGeneration set to True) to output a 
generatedSchema.usda and plugInfo.json.

The json config can provide sdrNodes either using sourceType and
identifiers or using explicit paths via sourceAssetNodes. Note that
if explicit paths contain any environment variables, then the user 
is required to set these prior to running the script. Example:
"$RMANTREE/lib/defaults/PRManAttribute.args", will require setting
the RMANTREE environment variable before running the script.

If regenerating schemas, it's recommended to set the
USD_DISABLE_AUTO_APPLY_API_SCHEMAS environment variable to true in 
order to prevent any previously generated auto-apply API schemas 
from being applied to the specified schema bases which can result 
in extra properties being pruned.

Note that since users of this script have less control on direct
authoring of schema.usda, "useLiteralIdentifier" is unconditionally
set to true in schema.usda, which means the default camelCase token 
names will be overriden and usdGenSchema will try keep the token 
names as-is unless these are invalid.

