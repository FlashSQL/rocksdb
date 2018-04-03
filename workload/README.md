# Workload for *ASLog*

## Synthetic Workload Data
Refer to [synthetic](synthetic)

1. **KEY**
- Structure: String [Timestamp:field1:field2:field3:field4]
- Timestamp: Unique YYYYMMDDhhmm (Interval: 10 minutes)
- &#35; of fields: 4
- Field1: Unique random [0000 - 9999]
- Field2: Unique random [0 - 9]
- Field3: Unique random [0 - 9]
- Field4: Unique random [0 - 199]
- &#35; of keys: 400K for each keys (Unique)

2. **VALUE**
- size: 100 bytes - 1 KiB Random value
- Test &#35;1: Fully random size
