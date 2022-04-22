
The files ("schema.usda", "generatedSchema.usda" and
"plugInfo.json") in this directory are auto generated using 
usdgenschemafromsdr utility.

A schema.usda is populated using sdrNodes which are specified in a
json config. usdGenSchema is then run on this auto populated schema 
(with skipCodeGeneration set to True) to output a 
generatedSchema.usda and plugInfo.json.

Note that since users of this script have less control on direct
authoring of schema.usda, "useLiteralIdentifier" is unconditionally
set to true, which means the default camelCase token names will be
overriden and usdGenSchema will try keep the token names as-is
unless these are invalid.
