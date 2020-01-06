# Development Practices For usdview

## Modifying GUI

     **Please do not hand edit .ui files, as qt's xml formt is fragile and its
     tools are inconsistent across platforms on tolerance to bad constructs.**
     
     Instead prefer qdesigner5 to make edits to the .ui documents.  If you are 
     allergic to gui tools, then **at least**, after making hand edits, prior to
     checking them in, load the file in qdesigner5, ensure it loads without 
     error, and then **save the file back out, and check that in**, so that the
     next developer using qdesigner5 does not contend with unrelated diffs.
     


## Testing

- Please run the tests in pxr/usdImaging/bin/testusdview, and add new ones
  whenever possible.
- See blackBoxTesting.md, and please add to it when adding features that
  cannot be tested by testusdview
