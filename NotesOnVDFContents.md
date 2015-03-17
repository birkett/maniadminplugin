#_486i extension for Legacy and OB VDF Files_

# Introduction #

With the TF2 & DODS patch in April 2010 the naming of the plugin within the VDF file has changed


# Details #

For the Legacy Source engine (CSS, HL2 etc) the _i486 is automatically appended to whatever you use in the VDF file_

```
"Plugin"
{
        "file" "../cstrike/addons/mani_admin_plugin"
}
```

This means that the game server will look for mani\_admin\_plugin\_i486.so as it automatically adds _i486 to the end_

For the Orange Box engine this is no longer the case and you have to explicitly give the whole name of the plugin (minus the .so suffix)

```
"Plugin"
{
        "file" "../cstrike/addons/mani_admin_plugin_i486"
}
```