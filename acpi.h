#include "config.h"
#include "types.h"

#define ACPI_RSDP_SIGNATURE "RSD PTR "
#define ACPI_RSDP_SIGNATURE_LEN 8
#define ACPI_OEM_ID_SIZE 6
#define ACPI_RSDP_CHECKSUM_LEN 20
#define ACPI_RSDP_XCHECKSUM_LEN 36
#define ACPI_MADT_SIGNATURE "APIC"
#define ACPI_MADT_SIGNATURE_LEN 4

typedef struct {
	char signature[8];
	u8 checksum;
	char oem_id[ACPI_OEM_ID_SIZE];
	u8 revision;
	u32 rsdt_physical_address;

	/*
	 * These are only accessible if revision >=2
	 */
	u32 length;
	u64 xsdt_physical_address;
	u8 extended_checksum;
	u8 reserved[3];
} acpi_table_rsdp;

typedef struct {
	// RSDT/XSTD header
	struct {
		char signature[4];
		u32 length;
		u8 revision;
		u8 checksum;
		char oem_id[6];
		char oem_table_id[8];
		u32 oem_revision;
		u32 creator_id;
		u32 creator_revision;
	} header;

	union {
		// If this is RDST:
		// Array of 32-bit pointers to other SDTs
		// of size (length - sizeof(header)) / 4;
		u32 sdts32[0];

		// If this is XDST:
		// Array of 64-bit pointers to other SDTs
		// of size (length - sizeof(header)) / 8;
		u64 sdts64[0];
	};

} acpi_table_rsdt;

typedef struct {
	// Header
	u8 signature[4];
	u32 length;
	u8 revision;
	u8 checksum;
	char oem_id[6];
	char oem_table_id[8];
	u32 oem_revision;
	u32 creator_id;
	u32 creator_revision;

	// MADT data
	u32 local_apic_address;
	u32 flags;
	/*
	 * Variable sized buffer that contains device info.
	 * Length of the buffer can be obtained from the header, from length
	 * field after subtracting amount of bytes occupied by fields that come
	 * before this one.  Device info is a sequence of header + payload,
	 * where each header is:
	 *
	 * struct {
	 *     u8 type;
	 *     u8 length;
	 * };
	 *
	 * Contents of data after the header is dependent on the type found in
	 * the header and length of the whole device info is in the length
	 * field.
	 */
	u8 device_info[0];
} acpi_table_madt;

struct apic_info {
	u64 local_apic_ptr;
	u64 ioapic_ptr;
	u8 lapic_ids[CONFIG_MAX_CPUS];
	u8 cpu_count;
	u8 ioapic_irqbase;
	u8 ioapic_id;
	u8 entries_skipped;
};

const acpi_table_rsdp* acpi_scan_bios_for_rsdp(void);
struct apic_info acpi_parse_madt(const acpi_table_rsdt *rsdt);

