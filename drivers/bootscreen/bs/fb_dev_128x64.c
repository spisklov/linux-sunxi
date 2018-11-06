// SPDX-License-Identifier: GPL-2.0

#include "bs_types.h"
#include "fb_dev_128x64.h"
#include "log.h"

#include "linux/fb.h"
#include "linux/slab.h"
#include <linux/uaccess.h>
#include "linux/workqueue.h"


#define BUF_SIZE (128 * 64)

struct fb_dev_128x64 {
	struct bs_fb_device device;
	struct work_struct ws;
	struct fb_ops fb_ops;
	u8 *buf;
	struct fb_info *info;
};

static struct fb_fix_screeninfo fb_fix = {
	.id         = "Solomon ssd1306",
	.type       = FB_TYPE_PACKED_PIXELS,
	.visual     = FB_VISUAL_MONO10,
	.xpanstep   = 0,
	.ypanstep   = 0,
	.ywrapstep  = 0,
	.accel      = FB_ACCEL_NONE,
};

static struct fb_var_screeninfo fb_var = {
	.bits_per_pixel = 1,
};


static ssize_t write(struct fb_info *info, const char __user *buf
	, size_t count, loff_t *ppos)
{
	struct fb_dev_128x64 *device = info->par;

	unsigned long total_size;
	unsigned long p = *ppos;
	u8 __iomem *dst;

	total_size = info->fix.smem_len;

	if (p > total_size)
		return -EINVAL;

	if (count + p > total_size)
		count = total_size - p;

	if (!count)
		return -EINVAL;

	dst = (void __force *) (info->screen_base + p);

	if (copy_from_user(dst, buf, count))
		return -EFAULT;

	schedule_work(&device->ws);

	*ppos += count;

	return count;

}


static int blank(int blank_mode, struct fb_info *info)
{
	struct fb_dev_128x64 *thiz = info->par;

	schedule_work(&thiz->ws);
	return 0;
}


static void fillrect(struct fb_info *info, const struct fb_fillrect *rect)
{
	struct fb_dev_128x64 *thiz = info->par;

	sys_fillrect(info, rect);
	schedule_work(&thiz->ws);
}


static void copyarea(struct fb_info *info, const struct fb_copyarea *area)
{
	struct fb_dev_128x64 *thiz = info->par;

	sys_copyarea(info, area);
	schedule_work(&thiz->ws);
}


static void imageblit(struct fb_info *info, const struct fb_image *image)
{
	struct fb_dev_128x64 *thiz = info->par;

	sys_imageblit(info, image);
	schedule_work(&thiz->ws);
}


static void update_display(struct work_struct *ws)
{
	struct fb_dev_128x64 *thiz = container_of(ws, struct fb_dev_128x64, ws);

	thiz->device.cleint->display(thiz->buf, BUF_SIZE);
}


static void destroy(struct bs_fb_device *device)
{
	struct fb_dev_128x64 *thiz = (struct fb_dev_128x64 *)device;

	cancel_work_sync(&thiz->ws);

	unregister_framebuffer(thiz->info);
	kfree(thiz->buf);

	framebuffer_release(thiz->info);
}


struct bs_fb_device *create_fb_dev_128x64(const struct bs_client *client)
{
	struct fb_info *info;
	struct fb_dev_128x64 *dev;
	int ret;

	info = framebuffer_alloc(sizeof(*dev), client->device);
	if (!info) {
		LOG(KERN_ERR, "failed to allocate memory for fb device");
		return NULL;
	}

	dev = info->par;
	dev->info = info;
	dev->buf = kzalloc(client->resolution.width * client->resolution.height
		, GFP_KERNEL);

	dev->device.cleint = client;
	dev->device.destroy = destroy;

	dev->fb_ops.owner = THIS_MODULE;
	dev->fb_ops.fb_read = fb_sys_read;
	dev->fb_ops.fb_write = write;
	dev->fb_ops.fb_blank = blank;
	dev->fb_ops.fb_fillrect = fillrect;
	dev->fb_ops.fb_copyarea = copyarea;
	dev->fb_ops.fb_imageblit = imageblit;
	info->fbops = &dev->fb_ops;

	info->fix = fb_fix;
	info->fix.line_length = client->resolution.width;

	info->var = fb_var;
	info->var.xres = client->resolution.width;
	info->var.xres_virtual = client->resolution.width;
	info->var.yres = client->resolution.height;
	info->var.yres_virtual = client->resolution.height;
	info->var.red.length = 1;
	info->var.red.offset = 0;
	info->var.green.length = 1;
	info->var.green.offset = 0;
	info->var.blue.length = 1;
	info->var.blue.offset = 0;

	info->screen_base = (u8 __force __iomem *)dev->buf;
	info->fix.smem_start = __pa(dev->buf);
	info->fix.smem_len = BUF_SIZE;

	ret = register_framebuffer(info);
	if (ret) {
		LOG(KERN_ERR, "failed to register framebuffer");
		destroy((struct bs_fb_device *)dev);
		return NULL;
	}

	INIT_WORK(&dev->ws, update_display);

	return (struct bs_fb_device *)dev;
}
