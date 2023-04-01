#include "acpi.h"
#include "types.h"
#include "std.h"

// Some BIOSes store segmented pointer to RSDP at that offset
#define BIOS_RSDP_SEGPTR_ADDRESS 0x40e

static u8 calculate_checksum(const acpi_table_rsdp* rsdp, u32 size)
{
	u8 checksum = 0;
	const u8 *data = (const u8*)rsdp;

	while(size--)
		checksum += *(data++);

	return checksum;
}

const acpi_table_rsdp* acpi_scan_bios_for_rsdp(void)
{
	const u64 BIOS_RSDP_SEARCH_MEM_START = 0xE0000;
	const u64 BIOS_RSDP_SEARCH_MEM_END   = 0xFFFFF;

	u64 address;
	for (address = BIOS_RSDP_SEARCH_MEM_START; address < BIOS_RSDP_SEARCH_MEM_END; ++address) {

		if (memcmp((const void*)address, ACPI_RSDP_SIGNATURE, ACPI_RSDP_SIGNATURE_LEN) != 0)
			continue;

		const acpi_table_rsdp *rsdp = (const acpi_table_rsdp*)address;

		/*
		 * Matching a signature string is not good enough. We have to
		 * make sure the checksum also matches because there might be a
		 * lot of places where this byte pattern exist.
		 *
		 * Actually byte pattern + valid checksum might exist as well,
		 * but what can we do..
		 */
		if (calculate_checksum(rsdp, ACPI_RSDP_CHECKSUM_LEN) != 0)
			continue;

		/* Check extended checksum if table version >= 2 */
		if ((rsdp->revision >= 2) && (calculate_checksum(rsdp, ACPI_RSDP_XCHECKSUM_LEN)))
			continue;

		return rsdp;
        }

	return NULL;
}
