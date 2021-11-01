
The files ("schema.usda", "generatedSchema.usda" and
"plugInfo.json") in this directory are auto generated using 
usdgenschemafromsdr utility.

A schema.usda is populated using sdrNodes which are specified in a
json config. usdGenSchema is then run on this auto populated schema 
(with skipCodeGeneration set to True) to output a 
generatedSchema.usda and plugInfo.json.
