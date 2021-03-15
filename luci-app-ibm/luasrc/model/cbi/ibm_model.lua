map = Map("ibm-conn")

section = map:section(NamedSection, "ibm_sct", "ibm", "IBM Cloud section")

flag = section:option(Flag, "enable", "Enable", "Enable connection")

org_id = section:option(Value, "org_id", "Organization ID")
org_id.datatype = "string"

dev_type = section:option(Value, "dev_type", "Device type ID")
dev_type.datatype = "string"

dev_id = section:option(Value, "dev_id", "Device ID")
dev_id.datatype = "string"

token = section:option(Value, "token", "Auth token")
token.datatype = "string"
token.maxlength = 100

return map