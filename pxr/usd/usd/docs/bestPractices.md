# Best Practices and Common Questions {#Usd_Page_BestPractices}

## Object Parameters as Const-Ref {#Usd_SharedConstRef}

## Reading Data Efficiently {#Usd_DataStreaming}

Processing different subtrees of a stage in different threads to reduce
contention for prims and paths.

Converting datatypes (VtValue::Cast) and potentially reading native datatypes. 

## Payloads: Impact of Using and Not Using {#Usd_Payloads}


## Miscellaneous Do's and Dont's {#Usd_Misc_Dos_Donts}

SdfPath::GetString() gives a fully unrolled, representation of any location
in namespace, and is very useful for debugging.  However, it is rather 
expensive, and may cause a globally-shared cache to be populated.  Don't call
it (or SdfPath::GetText()) in any performance-critical loops, and in general,
if using it for diagnostics, only call it inside a block that is guarded by
your debug flag.
