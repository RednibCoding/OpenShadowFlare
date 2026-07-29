# RKC_RPG_TABLE

This is the portable table-database boundary used by `SF_EXE`. It reads both
plain and RCLIB-L-compressed `TABLE DATA V000` files and keeps each table's
numeric and string cells available by table number.

The first consumer is `PlayerData`, which reads new-character parameters from
tables 900 and 901. The proven Win32 implementation under
`src/reconstructed/RKC_RPG_TABLE` remains the format and behavior reference;
this static library uses portable containers and does not preserve the DLL's
linked-list ABI.
