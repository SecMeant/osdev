#include "acpi.h"
#include "std.h"
#include "textmode.h"
#include "types.h"

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

static void acpi_parse_madt_devinfo(const u8 *devinfo, struct apic_info *ret)
{
	const u8 type = devinfo[0];
	switch(type) {

		/* Processor local APIC */
		case 0: {
				const u8 idcpu = devinfo[2];
				const u8 idapic = devinfo[3];
				//const u32 flags = *((u32*) &devinfo[4]);

				ret->lapic_ids[idcpu] = idapic;
				++ret->cpu_count;

				break;
			}

		/* IO APIC */
		case 1: {
				const u8 idapic = devinfo[2];
				const u32 address = *((u32*) &devinfo[4]);
				const u32 irqbase = *((u32*) &devinfo[8]);

				// FIXME: We are assigning the physical
				// address here and will treat this as
				// virtual, because we page first 1GB
				// that way, but it might break in the
				// future.
				ret->ioapic_ptr = (u64) address;

				ret->ioapic_irqbase = irqbase;
				ret->ioapic_id = idapic;

				break;
			}

		/* IO/APIC Interrupt Source Override */
		case 2: {
				const u8 bus_source = devinfo[2];
				const u8 irq_source = devinfo[3];
				const u32 global_sys_irq = *((u32*) &devinfo[4]);
				const u16 flags = *((u16*) &devinfo[8]);

				//txm_print(&earlytxm, "MADT: IRQ Source Override: ");
				//txm_line_feed(&earlytxm);
				//txm_print(&earlytxm, "    ");
				//txm_print_hex(&earlytxm, bus_source);
				//txm_line_feed(&earlytxm);
				//txm_print(&earlytxm, "    ");
				//txm_print_hex(&earlytxm, irq_source);
				//txm_line_feed(&earlytxm);
				//txm_print(&earlytxm, "    ");
				//txm_print_hex(&earlytxm, global_sys_irq);
				//txm_line_feed(&earlytxm);
				//txm_print(&earlytxm, "    ");
				//txm_print_hex(&earlytxm, flags);
				//txm_line_feed(&earlytxm);

				break;
			}

		/* Local APIC address override */
		case 5: {
				const u64 address = *((u64*) &devinfo[4]);

				ret->local_apic_ptr = address;

				break;
			}

		default:
			++ret->entries_skipped;
			break;
	}
}

struct apic_info acpi_parse_madt(const acpi_table_rsdt *rsdt)
{
	struct apic_info ret;

	memset(&ret, 0, sizeof(ret));

	// FIXME: This only work for RSDT and not XSDT
	const u32 sdt_size = sizeof(rsdt->sdts32[0]);
	const u32 sdts_count = (rsdt->header.length - sizeof(rsdt->header)) / sdt_size;
	for (u32 i = 0; i < sdts_count; ++i) {

		const void * const candidate = (const void*)((u64) rsdt->sdts32[i]);
		const u8 cmpres = memcmp(
			candidate,
			ACPI_MADT_SIGNATURE,
			ACPI_MADT_SIGNATURE_LEN
		);

		if (cmpres)
			continue;

		const acpi_table_madt * const madt = (const acpi_table_madt*) candidate;
		ret.local_apic_ptr = madt->local_apic_address;

		/*
		 * Parse all devices.
		 * Each device beings with a header:
		 *
		 * struct {
		 *     u8 type;
		 *     u8 length;
		 * };
		 *
		 */
		const u8 *       devinfo = &madt->device_info[0];
		const u8 * const devinfo_end = ((const u8*)madt) + madt->length;
		while (devinfo < devinfo_end) {
			const u8 length = devinfo[1];

			acpi_parse_madt_devinfo(devinfo, &ret);

			devinfo += length;
		}

		txm_line_feed(&earlytxm);

		/*
		 * We expect just one ACPI entry so as soon as we find it we
		 * get out of the loop.
		 */
		break;
	}

	return ret;
}

