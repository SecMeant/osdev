#include "types.h"

#define ACPI_RSDP_SIGNATURE "RSD PTR "
#define ACPI_RSDP_SIGNATURE_LEN 8
#define ACPI_OEM_ID_SIZE 6
#define ACPI_RSDP_CHECKSUM_LEN 20
#define ACPI_RSDP_XCHECKSUM_LEN 36

typedef struct {
	char signature[8];
	u8 checksum;
	char oem_id[ACPI_OEM_ID_SIZE];
	u8 revision;
	u32 rsdt_physical_address;
	u32 length;
	u64 xsdt_physical_address;
	u8 extended_checksum;
	u8 reserved[3];
} acpi_table_rsdp;

typedef struct {

} acpi_table_rsdt;

typedef struct {

} acpi_table_madt;

const acpi_table_rsdp* acpi_scan_bios_for_rsdp(void);
