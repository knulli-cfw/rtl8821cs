/*
 * Allwinner sunxi-rf glue (5.4 BSP API): the wlan node under rfkill owns the
 * module rail, the enable and host-wake pins; the driver only asks it to
 * power the chip and to rescan the SDIO host it is wired to.
 */
#include <drv_types.h>

extern void sunxi_mmc_rescan_card(unsigned ids);
extern void sunxi_wlan_set_power(bool on_off);
extern int sunxi_wlan_get_bus_index(void);
extern int sunxi_wlan_get_oob_irq(int *irq_flags, int *wakeup_enable);
#ifdef CONFIG_GPIO_WAKEUP
extern unsigned int oob_irq;
#endif

int platform_wifi_power_on(void)
{
	int bus = sunxi_wlan_get_bus_index();

	if (bus < 0)
		return bus;

	/*
	 * U-Boot leaves the module powered, so the host's first scan has
	 * usually enumerated it already.  Powering it up here resets the chip
	 * under that stale card object and every access then fails
	 * (bSurpriseRemoved) until the core has removed and re-added it, with
	 * the driver attaching to the wrong instance in between.  Take the
	 * module down and let the rescan drop the stale card first.
	 */
	sunxi_wlan_set_power(0);
	msleep(50);
	sunxi_mmc_rescan_card(bus);
	msleep(300);

	sunxi_wlan_set_power(1);
	msleep(100);
	sunxi_mmc_rescan_card(bus);
	RTW_INFO("%s: power up, rescan card on mmc%d\n", __func__, bus);

#ifdef CONFIG_GPIO_WAKEUP
	{
		int irq_flags = 0, wakeup = 0;

		oob_irq = sunxi_wlan_get_oob_irq(&irq_flags, &wakeup);
	}
#endif
	return 0;
}

void platform_wifi_power_off(void)
{
	int bus = sunxi_wlan_get_bus_index();

	if (bus < 0)
		return;

	sunxi_wlan_set_power(0);
	mdelay(100);
	sunxi_mmc_rescan_card(bus);
	RTW_INFO("%s: remove card, power off\n", __func__);
}
