#include "nandflash.h"
#include "timer.h"
#include "app_config.h"
#include "asm/clock.h"

#if TCFG_NANDFLASH_DEV_ENABLE

#undef LOG_TAG_CONST
#define LOG_TAG     "[nandFLASH]"
#define LOG_ERROR_ENABLE
#define LOG_INFO_ENABLE
#include "debug.h"



#define XT26G01C                    0x0b11
#define XT26G02C                    0x2c24
#define MX35LF2G14AC                0xc212
#define NAND_FLASH_TIMEOUT			500
#define MAX_NANDFLASH_PART_NUM      4




#define NAND_PAGE_SIZE              2048
#define NAND_BLOCK_SIZE             0x20000
#define NAND_PLANE_SELECT           6
#define NAND1G_CAPACITY             0X8000000
#define NAND2G_CAPACITY             0X10000000



struct nandflash_data {
    u16 page_size;
    u32 block_size;
    u8  plane_select;
    u32 capacity;
};

static struct nandflash_data nand_flash;

struct nandflash_partition {
    const char *name;
    u32 start_addr;
    u32 size;
    struct device device;
};

static struct nandflash_partition nor_part[MAX_NANDFLASH_PART_NUM];

struct nandflash_info {
    u32 flash_id;
    int spi_num;
    int spi_err;
    u8 spi_cs_io;
    u8 spi_r_width;
    u8 part_num;
    u8 open_cnt;
    struct nandflash_partition *const part_list;
    OS_MUTEX mutex;
    u32 max_end_addr;
};

static struct nandflash_info _nandflash = {
    .spi_num = (int) - 1,
    .part_list = nor_part,
};

int nand_flash_read(u32 addr, u8 *buf, u32 len);
int nand_flash_erase(u32 addr);


#define spi_cs_init() \
    do { \
        gpio_set_die(_nandflash.spi_cs_io, 1); \
        gpio_set_direction(_nandflash.spi_cs_io, 0); \
        gpio_write(_nandflash.spi_cs_io, 1); \
    } while (0)

#define spi_cs_uninit() \
    do { \
        gpio_set_die(_nandflash.spi_cs_io, 0); \
        gpio_set_direction(_nandflash.spi_cs_io, 1); \
        gpio_set_pull_up(_nandflash.spi_cs_io, 0); \
        gpio_set_pull_down(_nandflash.spi_cs_io, 0); \
    } while (0)
#define spi_cs_h()                  gpio_write(_nandflash.spi_cs_io, 1)
#define spi_cs_l()                  gpio_write(_nandflash.spi_cs_io, 0)
#define spi_read_byte()             spi_recv_byte(_nandflash.spi_num, &_nandflash.spi_err)
#define spi_write_byte(x)           spi_send_byte(_nandflash.spi_num, x)
#define spi_dma_read(x, y)          spi_dma_recv(_nandflash.spi_num, x, y)
#define spi_dma_write(x, y)         spi_dma_send(_nandflash.spi_num, x, y)
#define spi_set_width(x)            spi_set_bit_mode(_nandflash.spi_num, x)

static struct nandflash_partition *nandflash_find_part(const char *name)
{
    struct nandflash_partition *part = NULL;
    u32 idx;
    for (idx = 0; idx < MAX_NANDFLASH_PART_NUM; idx++) {
        part = &_nandflash.part_list[idx];
        if (part->name == NULL) {
            continue;
        }
        if (!strcmp(part->name, name)) {
            return part;
        }
    }
    return NULL;
}

static struct nandflash_partition *nandflash_new_part(const char *name, u32 addr, u32 size)
{
    struct nandflash_partition *part;
    u32 idx;
    for (idx = 0; idx < MAX_NANDFLASH_PART_NUM; idx++) {
        part = &_nandflash.part_list[idx];
        if (part->name == NULL) {
            break;
        }
    }
    if (part->name != NULL) {
        log_error("create nandflash part fail\n");
        return NULL;
    }
    memset(part, 0, sizeof(*part));
    part->name = name;
    part->start_addr = addr;
    part->size = size;
    if (part->start_addr + part->size > _nandflash.max_end_addr) {
        _nandflash.max_end_addr = part->start_addr + part->size;
    }
    _nandflash.part_num++;
    return part;
}

static void nandflash_delete_part(const char *name)
{
    struct nandflash_partition *part;
    u32 idx;
    for (idx = 0; idx < MAX_NANDFLASH_PART_NUM; idx++) {
        part = &_nandflash.part_list[idx];
        if (part->name == NULL) {
            continue;
        }
        if (!strcmp(part->name, name)) {
            part->name = NULL;
            _nandflash.part_num--;
        }
    }
}

static int nandflash_verify_part(struct nandflash_partition *p)
{
    struct nandflash_partition *part = NULL;
    u32 idx;
    for (idx = 0; idx < MAX_NANDFLASH_PART_NUM; idx++) {
        part = &_nandflash.part_list[idx];
        if (part->name == NULL) {
            continue;
        }
        if ((p->start_addr >= part->start_addr) && (p->start_addr < part->start_addr + part->size)) {
            if (strcmp(p->name, part->name) != 0) {
                return -1;
            }
        }
    }
    return 0;
}

u16 nand_flash_read_id()
{
    u16 id;
    os_time_dly(1);
    spi_cs_l();
    spi_write_byte(GD_READ_ID);
    spi_write_byte(0x0);
    id = spi_read_byte();
    id <<= 8;
    id |= spi_read_byte();
    spi_cs_h();
    return id;
}

static void nand_write_enable()
{
    spi_cs_l();
    spi_write_byte(GD_WRITE_ENABLE);
    spi_cs_h();
}

static void nand_write_disable()
{
    spi_cs_l();
    spi_write_byte(GD_WRITE_DISABLE);
    spi_cs_h();
}

static void nand_set_features(u8 addr, u8 dat)
{
    spi_cs_l();
    spi_write_byte(GD_SET_FEATURES); //命令头
    spi_write_byte(addr);             //发送protection寄存器地址
    spi_write_byte(dat);             //需要设置的数据
    spi_cs_h();
}

static u8 nand_get_features(u8 addr)
{
    u8 temp;
    spi_cs_l();
    spi_write_byte(GD_GET_FEATURES); //命令头
    spi_write_byte(addr);             //发送protection寄存器地址
    temp = spi_read_byte();
    spi_cs_h();
    return temp;
}
enum {
    NANDFLASH_OIP_FAIL = 1,
    NANDFLASH_E_FAIL,
    NANDFLASH_P_FAIL,
    NANDFLASH_ECC_FAIL,

};
static u8 nand_flash_wait_ok(u32 timeout)
{
    u8 sta;
    do {
        sta = nand_get_features(GD_GET_STATUS);
        if ((sta & NAND_STATUS_OIP) == 0) {
            sta = 0;
            break;
        }
#if 1
//It will also be set if the user attempts to program an invalid
//address or a locked or a protected region, including the OTP area.
        if (sta & NAND_STATUS_P_FAIL) { //ing
            sta = NANDFLASH_P_FAIL;
            log_error("nand_flash prom error!\r\n");
            goto _exit;
        }
//It will also be set if the user attempts to erase a locked region.
        if (sta & NAND_STATUS_E_FAIL) { //erase fail
            sta = NANDFLASH_E_FAIL;
            log_error("nand_flash erase error!\r\n");
            goto _exit;
        }
        if (_nandflash.flash_id == XT26G01C) { //XT26G01C(1Gb)
            if ((sta & 0xf0) == 0xf0) { //1Gb:ecc error. block data should be refreshed when ECC status is equal to 1111b.
                log_error("nand_flash block data should be refreshed!\r\n");
                sta = NANDFLASH_ECC_FAIL; //块数据错误
                break;
            }
        } else if (_nandflash.flash_id == XT26G02C) { //XT26G02C(2Gb)
            if (((sta >> 4) & 0x07) >= 2) { //2Gb:ecc error. block data should be refreshed when ECC status is equal to 111b.
                log_error("nand_flash block data should be refreshed!\r\n");
                sta = NANDFLASH_ECC_FAIL; //块数据错误
                break;
            }
        } else {
        }
#endif
    } while (timeout--);
    if (timeout == 0) {
        log_error("nand_flash_wait_ok timeout!\r\n");
        sta = NANDFLASH_OIP_FAIL;
    }
_exit:
    return sta;
}


void nand_flash_reset(void)
{
    /* mdelay(2);//2ms */
    os_time_dly(2);
    spi_cs_l();
    spi_write_byte(0xff); //命令头
    spi_cs_h();
    /* mdelay(2);//2ms */
    os_time_dly(2);
// 4Gb:must wait Tpor,2Gb:no wait
    /* mdelay(2);//2ms */
    os_time_dly(2);
// wait ok
    if (nand_flash_wait_ok(NAND_FLASH_TIMEOUT)) {
        log_error("nand flash reset error");
    }
}

static u32 block_page_get(u32 addr, u32 *cache_addr)
{
    u32  page = 0;
    u32 block = 0, bp_mix = 0;
    //<地址超1页范围
    //if (addr >= NAND_PAGE_SIZE) {
    if (addr >= nand_flash.page_size) {
        if (addr >= nand_flash.block_size) {
            while (addr >= nand_flash.block_size) {
                block++;
                addr -= nand_flash.block_size;
            }
            goto _page_count;
        } else {
            goto _page_count;
        }
    }
    //<地址不超1页范围
    else {
        goto _end_count;
    }

_page_count:
    while (addr >= nand_flash.page_size) {
        page++;
        addr -= nand_flash.page_size;
    }

_end_count:
    *cache_addr = addr;
    bp_mix = (block << nand_flash.plane_select) | page;
    log_info("addr change:block:%d,page:%d,cache:%d", block, page, addr);
    return bp_mix;
}

#define block_change(x) (x)

int nand_flash_erase(u32 address)
{
    u32 bp_mix = 0;
    u32 cache_addr;
    bp_mix = block_page_get(address, &cache_addr);
    nand_write_enable();
    spi_cs_l();
    spi_write_byte(GD_BLOCK_ERASE);
    spi_write_byte(bp_mix >> 16); //擦除地址:block+page addr
    spi_write_byte(bp_mix >> 8);//
    spi_write_byte(bp_mix);
    spi_cs_h();
    /* nand_write_disable(); */
    //delay 20ns  erase time:4~10ms
    if (nand_flash_wait_ok(NAND_FLASH_TIMEOUT)) {
        log_error("nand flash erase error");
    }
    return nand_get_features(GD_GET_STATUS);
}

static void nand_flash_erase_all()
{

    nand_set_features(0xA0, 0x00);
    for (int i = 0; i < nand_flash.capacity; i += nand_flash.block_size) {
        nand_flash_erase(i);
    }
    r_printf("nandflash erase all!!!");
}

static u8 nand_page_read_to_cache(u32 block_page_addr) //PageRead
{
    u8 reg = 0;
    spi_cs_l();
    spi_write_byte(GD_PAGE_READ_CACHE);         //PageRead to cache
    spi_write_byte(block_page_addr >> 16);  //send the Page/Block Add
    spi_write_byte((u8)((block_page_addr) >> 8));
    spi_write_byte((u8)block_page_addr);
    spi_cs_h();
    /* nand_flash_wait_ok(NAND_FLASH_TIMEOUT); */
    reg = nand_flash_wait_ok(NAND_FLASH_TIMEOUT);
    if (reg) {
        log_error("nand flash read to cache error");
    }
    return reg;
}

//03h/0bh/3bh/6bh/bbh/ebh:addr_bit12:sel plane
static void nand_read_from_cache(u8 *buf, u32 cache_addr, u32 len) //ReadCache
{
    /* cache_addr |= 0x4000; */
    spi_cs_l();//SPI_CS(0);
    spi_write_byte(GD_READ_FROM_CACHE);     //PageRead to cache
    spi_write_byte((u8)((cache_addr) >> 8));
    spi_write_byte((u8)cache_addr);
    spi_write_byte(0xFF);  //send 1byte dummy clk
    spi_dma_read(buf, len);
    spi_cs_h();//SPI_CS(1);
}

static u8 nand_read(u32 addr, u32 len, u8 *buf)
{
    u8 reg = 0;
    u32 bp_mix = 0;
    u32 cache_addr;
    bp_mix = block_page_get(addr, &cache_addr);
    bp_mix = block_change(bp_mix);
    if (_nandflash.flash_id == XT26G01C) { //XT26G01C(1Gb)
        cache_addr &= 0x0fff;
    } else if (_nandflash.flash_id == XT26G02C || _nandflash.flash_id == MX35LF2G14AC) { //XT26G02C(2Gb)
        cache_addr &= 0x0fff;
        if (bp_mix & BIT(6)) {
            cache_addr |= BIT(12);
        }
    }
    reg = nand_page_read_to_cache(bp_mix);
    if (reg) {
        return reg;
    }
    //时间间隔需小于trd
    nand_read_from_cache(buf, cache_addr, len);
    return reg;
}

//简化接口
int nand_flash_read(u32 offset, u8 *buf,  u32 len)
{
    /* printf("%s() %x l: %x @:%x \n",__func__,buf,len,offset); */
    int reg = 0;
    int _len = len;
    u8 *_buf = (u8 *)buf;
    os_mutex_pend(&_nandflash.mutex, 0);
    u32 first_page_len = nand_flash.page_size - (offset % nand_flash.page_size);
    /* printf("first_page_len %x %x \n", first_page_len, offset % NAND_PAGE_SIZE); */
    first_page_len = _len > first_page_len ? first_page_len : _len;
    reg = nand_read(offset, first_page_len, _buf);
    if (reg) {
        log_error("read nandflash addr:%d fail!", offset);
        goto __read_exit;
    }
    _len -= first_page_len;
    _buf += first_page_len;
    offset += first_page_len;
    while (_len) {
        u32 cnt = _len > nand_flash.page_size ? nand_flash.page_size : _len;
        reg = nand_read(offset, cnt, _buf);
        if (reg) {
            log_error("read nandflash addr:%d fail!", offset);
            goto __read_exit;
        }
        _len -= cnt;
        offset += cnt;
        _buf += cnt;
    }
__read_exit:
    os_mutex_post(&_nandflash.mutex);
    return reg;
}

//2Gb-nandflash random read(30h,3fh):

//write:
#if 1
//1Gb 2Gb program 时序不同
//Only four partial-page programs(02h) are allowed on a single page.*******************
//02h/32h/84h/34h:addr_bit12:sel plane
static void nand_program_load(u8 *buf, u32 cache_addr, u16 len)
{
    spi_cs_l();//SPI_CS(0);
    spi_write_byte(GD_PROGRAM_LOAD);         //PageLoad to cache ,change the data
    spi_write_byte((u8)((cache_addr) >> 8));
    spi_write_byte((u8)cache_addr);
    spi_dma_write(buf, len); //将数据放到总线上
    spi_cs_h();//SPI_CS(1);
}

static int nand_program_excute(u32 block_page_addr)
{
    int reg;
    spi_cs_l();
    spi_write_byte(GD_PROGRAM_EXECUTE);
    spi_write_byte(block_page_addr >> 16);  //send the Page/Block Add
    spi_write_byte((u8)((block_page_addr) >> 8));
    spi_write_byte((u8)block_page_addr);
    spi_cs_h();
    reg = nand_flash_wait_ok(NAND_FLASH_TIMEOUT);
    if (reg) {
        log_error("nand flash program error");
    }
    return reg;
}


static int nand_write(u32 addr, u16 len, u8 *buf)
{
    int reg;
    u32 bp_mix = 0;
    u32 cache_addr;

    bp_mix = block_page_get(addr, &cache_addr);
    bp_mix = block_change(bp_mix);
    //printf_u16(cache_addr);
    if (_nandflash.flash_id == XT26G01C) { //XT26G01C(1Gb)
        cache_addr &= 0x0fff;
    } else if (_nandflash.flash_id == XT26G02C || _nandflash.flash_id == MX35LF2G14AC) { //XT26G02C(2Gb)
        cache_addr &= 0x0fff;
        if (bp_mix & BIT(6)) {
            cache_addr |= BIT(12);
        }
        nand_write_enable();
    }
    nand_program_load(buf, cache_addr, len);
    if (_nandflash.flash_id == XT26G01C) { //XT26G01C(1Gb)
        nand_write_enable();
    }
    reg = nand_program_excute(bp_mix);
    return reg;
}/**/
//写前需确保该block(128k)已擦除
int nand_flash_write(u32 offset, u8 *buf,  u32 len)
{
    /* printf("%s() %x l: %x @:%x \n",__func__,buf,len,offset); */
    int reg;
    int _len = len;
    u8 *_buf = (u8 *)buf;
    os_mutex_pend(&_nandflash.mutex, 0);
    u32 first_page_len = nand_flash.page_size - (offset % nand_flash.page_size);
    /* printf("first_page_len %x %x \n", first_page_len, offset % NAND_PAGE_SIZE); */
    first_page_len = _len > first_page_len ? first_page_len : _len;
    reg = nand_write(offset, first_page_len, _buf);
    if (reg) {
        goto __exit;
    }
    _len -= first_page_len;
    _buf += first_page_len;
    offset += first_page_len;
    while (_len) {
        u32 cnt = _len > nand_flash.page_size ? nand_flash.page_size : _len;
        reg = nand_write(offset, cnt, _buf);
        if (reg) {
            goto __exit;
        }
        _len -= cnt;
        offset += cnt;
        _buf += cnt;
    }
__exit:
    os_mutex_post(&_nandflash.mutex);
    return reg;
}

//1Gb 2Gb program 时序不同
// If the random data is not sequential, then another PROGRAM LOAD RANDOM DATA x1 (84h) command must be issued with a new column address.*******************
//02h/32h/84h/34h:addr_bit12:sel plane
static void nand_program_load_random_data(u8 *buf, u16 cache_addr, u16 len)
{
    spi_cs_l();//SPI_CS(0);
    spi_write_byte(GD_PROGRAM_LOAD_RANDOM_DATA);
    spi_write_byte((u8)((cache_addr) >> 8));
    spi_write_byte((u8)cache_addr);
    spi_dma_write(buf, len); //将数据放到总线上
    spi_cs_h();//SPI_CS(1);
}

/*
 * 块内页数据移动
 * page_src_addr :要移动的页
 * page_dest_addr:移动目标页(需已擦除)
 * offset :需要替换的数据起始地址
 * len    :需替换数据长度. 无则=0
 * buf    :存储的替换的数据.无则NULL
 * */
int nand_page_internal_data_move(u32 page_src_addr, u32 page_dest_addr, u16 offset, u16 len, u8 *buf)
{
    u32 bp_mix_src = 0, bp_mix_dst = 0;
    u32 cache_addr_src, cache_addr_dst;

    bp_mix_src = block_page_get(page_src_addr, &cache_addr_src);
    bp_mix_src = block_change(bp_mix_src);
    bp_mix_dst = block_page_get(page_dest_addr, &cache_addr_dst);
    bp_mix_dst = block_change(bp_mix_dst);

    //<1、PAGE READ to cache
    nand_page_read_to_cache(bp_mix_src);
    //<2、PROGRAM LOAD RANDOM DATA
    if (_nandflash.flash_id == XT26G01C) { //XT26G01C(1Gb)
        cache_addr_src &= 0x0fff;
    } else if (_nandflash.flash_id == XT26G02C || _nandflash.flash_id == MX35LF2G14AC) { //XT26G02C(2Gb)
        cache_addr_src &= 0x0fff;
        if (bp_mix_src & BIT(6)) {
            cache_addr_src |= BIT(12);
        }
        nand_write_enable();
    }
    if (buf && (len > 0)) {
        nand_program_load_random_data(buf, cache_addr_src + offset, len);
    }
// If the random data is not sequential, then another PROGRAM LOAD RANDOM DATA x1 (84h) command must be issued with a new column address.*******************
    //<3、WRITE ENABLE
    //<4、PROGRAM EXECUTE
    if (_nandflash.flash_id == XT26G01C) { //XT26G01C(1Gb)
        nand_write_enable();
    }
    nand_program_excute(bp_mix_dst);
    //<5、GET FEATURE
    return nand_get_features(GD_GET_STATUS);
}

#endif


int _nandflash_init(const char *name, struct nandflash_dev_platform_data *pdata)
{
    log_info("nandflash_init ! %x %x", pdata->spi_cs_port, pdata->spi_read_width);
    if (_nandflash.spi_num == (int) - 1) {
        _nandflash.spi_num = pdata->spi_hw_num;
        _nandflash.spi_cs_io = pdata->spi_cs_port;
        _nandflash.spi_r_width = pdata->spi_read_width;
        _nandflash.flash_id = 0;
        os_mutex_create(&_nandflash.mutex);
        _nandflash.max_end_addr = 0;
        _nandflash.part_num = 0;
    }
    ASSERT(_nandflash.spi_num == pdata->spi_hw_num);
    ASSERT(_nandflash.spi_cs_io == pdata->spi_cs_port);
    ASSERT(_nandflash.spi_r_width == pdata->spi_read_width);
    struct nandflash_partition *part;
    part = nandflash_find_part(name);
    if (!part) {
        part = nandflash_new_part(name, pdata->start_addr, pdata->size);
        ASSERT(part, "not enough nandflash partition memory in array\n");
        ASSERT(nandflash_verify_part(part) == 0, "nandflash partition %s overlaps\n", name);
        log_info("nandflash new partition %s\n", part->name);
    } else {
        ASSERT(0, "nandflash partition name already exists\n");
    }
    return 0;
}

static void clock_critical_enter()
{

}
static void clock_critical_exit()
{
    if (!(_nandflash.flash_id == 0 || _nandflash.flash_id == 0xffff)) {
        spi_set_baud(_nandflash.spi_num, spi_get_baud(_nandflash.spi_num));
    }
}
CLOCK_CRITICAL_HANDLE_REG(spi_nandflash, clock_critical_enter, clock_critical_exit);

//2Gb上电自动复位,1Gb未介绍
int _nandflash_open(void *arg)
{
    int reg = 0;
    os_mutex_pend(&_nandflash.mutex, 0);
    log_info("nandflash open\n");
    if (!_nandflash.open_cnt) {
        spi_cs_init();
        spi_open(_nandflash.spi_num);
// nand flash power-up need wait 1.25ms
// wait oip ok
        if (nand_flash_wait_ok(NAND_FLASH_TIMEOUT)) {
            log_error("nand flash power-up error");
            reg = -ENODEV;
            goto __exit;
        }

        _nandflash.flash_id = nand_flash_read_id();
        if (_nandflash.flash_id == XT26G01C) { //XT26G01C(1Gb)
            nand_flash.page_size = NAND_PAGE_SIZE;
            nand_flash.block_size = NAND_BLOCK_SIZE;
            nand_flash.capacity = NAND1G_CAPACITY;
            nand_flash.plane_select = NAND_PLANE_SELECT;
        } else if (_nandflash.flash_id == XT26G02C || _nandflash.flash_id == MX35LF2G14AC) { //XT26G02C(2Gb)
            nand_flash.page_size = NAND_PAGE_SIZE;
            nand_flash.block_size = NAND_BLOCK_SIZE;
            nand_flash.capacity = NAND2G_CAPACITY;
            nand_flash.plane_select = NAND_PLANE_SELECT;
        }
        //power-up:need set featurea[A0]=00h
        nand_set_features(0xA0, 0x00);
        log_info("nandflash_read_id: 0x%x\n", _nandflash.flash_id);
        if ((_nandflash.flash_id == 0) || (_nandflash.flash_id == 0xffff)) {
            log_error("read nandflash id error !\n");
            reg = -ENODEV;
            goto __exit;
        }
        log_info("nandflash open success !\n");
    }
    if (_nandflash.flash_id == 0 || _nandflash.flash_id == 0xffff)  {
        log_error("re-open nandflash id error !\n");
        reg = -EFAULT;
        goto __exit;
    }
    _nandflash.open_cnt++;
    //初始化时,擦除nandflash
    /* nand_flash_erase_all();// */
__exit:
    os_mutex_post(&_nandflash.mutex);
    return reg;
}

int _nandflash_close(void)
{
    os_mutex_pend(&_nandflash.mutex, 0);
    log_info("nandflash close\n");
    if (_nandflash.open_cnt) {
        _nandflash.open_cnt--;
    }
    if (!_nandflash.open_cnt) {
        spi_close(_nandflash.spi_num);
        spi_cs_uninit();

        log_info("nandflash close done\n");
    }
    os_mutex_post(&_nandflash.mutex);
    return 0;
}


int _nandflash_ioctl(u32 cmd, u32 arg, u32 unit, void *_part)
{
    int reg = 0;
    struct nandflash_partition *part = _part;
    os_mutex_pend(&_nandflash.mutex, 0);
    switch (cmd) {
    case IOCTL_GET_STATUS:
        *(u32 *)arg = nand_get_features(GD_GET_STATUS);
        break;
    case IOCTL_GET_ID:
        *((u32 *)arg) = _nandflash.flash_id;
        break;
    case IOCTL_GET_BLOCK_SIZE:
        *(u32 *)arg = nand_flash.block_size;
        break;
    case IOCTL_ERASE_BLOCK:
        reg = nand_flash_erase(arg  + part->start_addr);
        break;
    case IOCTL_GET_CAPACITY:
        *(u32 *)arg = nand_flash.capacity;
        break;
    case IOCTL_FLUSH:
        break;
    case IOCTL_CMD_RESUME:
        break;
    case IOCTL_CMD_SUSPEND:
        break;
    default:
        reg = -EINVAL;
        break;
    }
__exit:
    os_mutex_post(&_nandflash.mutex);
    return reg;
}


/*************************************************************************************
 *                                  挂钩 device_api
 ************************************************************************************/

static int nandflash_dev_init(const struct dev_node *node, void *arg)
{
    struct nandflash_dev_platform_data *pdata = arg;
    return _nandflash_init(node->name, pdata);
}

static int nandflash_dev_open(const char *name, struct device **device, void *arg)
{
    struct nandflash_partition *part;
    part = nandflash_find_part(name);
    if (!part) {
        log_error("no nandflash partition is found\n");
        return -ENODEV;
    }
    *device = &part->device;
    (*device)->private_data = part;
    if (atomic_read(&part->device.ref)) {
        return 0;
    }
    return _nandflash_open(arg);
}
static int nandflash_dev_close(struct device *device)
{
    return _nandflash_close();
}
static int nandflash_dev_read(struct device *device, void *buf, u32 len, u32 offset)
{
    int reg;
    offset = offset * 512;
    len = len * 512;
    /* r_printf("flash read sector = %d, num = %d\n", offset, len); */
    struct nandflash_partition *part;
    part = (struct nandflash_partition *)device->private_data;
    if (!part) {
        log_error("nandflash partition invalid\n");
        return -EFAULT;
    }
    offset += part->start_addr;
    reg = nand_flash_read(offset, buf, len);
    if (reg) {
        r_printf(">>>[r error]:\n");
        len = 0;
    }
    len = len / 512;
    return len;
}
//写前需确保该block(128k)已擦除
static int nandflash_dev_write(struct device *device, void *buf, u32 len, u32 offset)
{
    /* r_printf("flash write sector = %d, num = %d\n", offset, len); */
    int reg = 0;
    offset = offset * 512;
    len = len * 512;
    struct nandflash_partition *part = device->private_data;
    if (!part) {
        log_error("nandflash partition invalid\n");
        return -EFAULT;
    }
    offset += part->start_addr;
    reg = nand_flash_write(offset, buf, len);
    if (reg) {
        r_printf(">>>[w error]:\n");
        len = 0;
    }
    len = len / 512;
    return len;
}
static bool nandflash_dev_online(const struct dev_node *node)
{
    return 1;
}
static int nandflash_dev_ioctl(struct device *device, u32 cmd, u32 arg)
{
    struct nandflash_partition *part = device->private_data;
    if (!part) {
        log_error("nandflash partition invalid\n");
        return -EFAULT;
    }
    return _nandflash_ioctl(cmd, arg, 512,  part);
}
const struct device_operations nandflash_dev_ops = {
    .init   = nandflash_dev_init,
    .online = nandflash_dev_online,
    .open   = nandflash_dev_open,
    .read   = nandflash_dev_read,
    .write  = nandflash_dev_write,//写前需确保该block(128k)已擦除
    .ioctl  = nandflash_dev_ioctl,
    .close  = nandflash_dev_close,
};


void nandflash_test_demo(void)
{
    u8 test_read_buf[10] = {0};
    u8 test_write_buf[10] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0x00};
    u8 test_move_buf[5] = {0x10, 0x20, 0x30, 0x40, 0x50};
    _nandflash_open(NULL);
    nand_set_features(GD_FEATURES_PROTECT, 0x00);
    printf("nandflash open success");
    nand_flash_erase(0x00);
    nand_write(0x00, 10, test_write_buf);
    nand_read(0x00, 10, test_read_buf);
    if (!memcmp(test_read_buf, test_write_buf, 10)) {
        printf("write flash success");
        printf("data is ");
        printf_buf(test_read_buf, 10);
    } else {
        printf("write flash fail");
        printf_buf(test_read_buf, 10);
    }
    nand_page_internal_data_move(0x00, 0x800, 0x00, 5, test_move_buf);
    nand_read(0x800, 10, test_read_buf);
    printf("test move buf is ");
    printf_buf(test_read_buf, 10);
}

#endif




