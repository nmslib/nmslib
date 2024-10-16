
/* EMAX7 library                        */
/*         Copyright (C) 2013- by NAIST */
/*          Primary writer: Y.Nakashima */
/*                 nakashim@is.naist.jp */

/*******************************************************************************/
/******************************** Defs *****************************************/
/*******************************************************************************/

enum { NANOS_ARM, NANOS_DRAIN, NANOS_CONF, NANOS_REGV, NANOS_RANGE, NANOS_LOAD, NANOS_EXEC, NANOS_TOTAL, NANOS_CLASS };

typedef struct {
  Uint  f : 23;
  Uint  e :  8;
  Uint  s :  1;
} f32bit;

typedef struct {
  Uint  e :  6;
  Uint  b :  1;
  Uint  s :  1; /* lower 8bit */
  Uint dm : 24; /* dummy for >gcc9 */
} wu7bit;

typedef struct {
  Uint  e :  7;
  Uint  s :  1; /* lower 8bit */
  Uint dm : 24; /* dummy for >gcc9 */
} wu8bit;

typedef struct {
  Uchar u[8];
} u64bit;

#define abs(a)    ((a)>  0 ? (a) :-(a)    )
#define ad(a,b)   ((a)<(b)?(b)-(a):(a)-(b))
#define ss(a,b)   ((a)<(b)?   0   :(a)-(b))

/* dma_ctrl_space */
/* https://www.xilinx.com/content/dam/xilinx/support/documents/ip_documentation/axi_dma/v7_1/pg021_axi_dma.pdf */
struct dma_ctrl {
  /*   Register Name		   Address	Width	Type	Reset Value	Description */
  Uint MM2S_DMACR;             /* 0x00000000��  32      mixed   0x00010000      DMA Control register */
	/*   Field Name    Bits  Type Default Val  Description            */
	/*   Run/Stop         0  rw   0x0   ��	   0:stop-DMA,1:start-DMA */
	/*   Reserved 	      1  ro   0x1	   Reserved for future use */
        /*   Reset            2  rw   0x0          0:normal, 1:reset in progress */
        /*   Keyhole          3  rw   0x0          0:normal, 1:non-incrementing addr */
        /*   Cycle BD En      4  rw   0x0          0:normal, 1:Cycle Buffer Descriptor */
        /*   Reserved      11-5  ro   0x0          Reserved for future use */
        /*   IOC_IrqEn       12  rw   0x0          0:IOC   Intr. disabled, 1:IOC   Intr. enabled */
        /*   Dly_IrqEn       13  rw   0x0          0:Delay Intr. disabled, 1:Delay Intr. enabled */
        /*   Err_IrqEn       14  rw   0x0          0:Error Intr. disabled, 1:Error Intr. enabled */
        /*   Reserved        15  ro   0x0          Reserved for future use */
        /*   IRQThreshold 23-16  rw   0x1          Intr. threshold */
        /*   IRQThreshold 31-24  rw   0x0          Intr. delay time out */

  Uint MM2S_DMASR;             /* 0x00000004��  32      mixed   0x00010000      DMA Status register */
	/*   Field Name    Bits  Type Default Val  Description            */
	/*   Halted           0  ro   0x1   ��     0:DMA channel running, 1:DMA channel halted */
	/*   Idle             1  ro   0x0   ��     0:not Idle, 1:Idle */
        /*   Reserved         2  ro   0x0          Reserved for future use */
        /*   SGIncld          3  ro   C_INCLUDE_SG 0:SG N.A, 1:SG enabled */
        /*   DMAIntErr        4  ro   0x0   ��     0:no DMA internal error, 1:DMA internal error */
        /*   DMASlvErr        5  ro   0x0   ��     0:no DMA slave errors,   1:DMA slave error */
        /*   DMADecErr        6  ro   0x0   ��     0:no DMA decode errors,  1:DMA decode error (invalid address) */
        /*   Reserved         7  ro   0x0          Reserved for future use */
        /*   SGIntErr         8  ro   0x0          0:no SG internal error,  1:SG internal error */
        /*   SGSlvErr         9  ro   0x0          0:no SG slave errors,    1:SG slave error */
        /*   SGDecErr        10  ro   0x0          0:no SG decode errors,   1:SG decode error (invalid address) */
        /*   Reserved        11  ro   0x0          Reserved for future use */
        /*   IOC_Irq         12  rwc  0x0          0:no IOC intr.   1:IOC intr. */
        /*   Dly_Irq         13  rwc  0x0          0:no Delay intr. 1:Delay intr. */
        /*   Err_Irq         14  rwc  0x0          0:no Err intr.   1:Err intr. */
        /*   Reserved        15  ro   0x0          Reserved for future use */
        /*   IRQThreshold 23-16  ro   0x1          Intr. threshold stat */
        /*   IRQThreshold 31-24  ro   0x0          Intr. delay time out stat */

  Uint reserved0[4];           /* 08h - 14h Reserved N/A */
  Uint MM2S_SA;                /* 0x00000018    32      rw      0x00000000      Source Address. Lower 32 bits of address.*/
  Uint MM2S_SA_MSB;            /* 0x0000001c    32      rw      0x00000000      Source Address. Upper 32 bits of address.*/
  Uint reserved1[2];           /* 20h - 24h Reserved N/A */
  Uint MM2S_LENGTH;            /* 0x00000028    32      rw      0x00000000      Transfer Length (Bytes) */
  Uint reserved2[1];           /* 2ch       Reserved N/A */
  Uint S2MM_DMACR;             /* 0x00000030��  32      mixed   0x00010000      DMA Control register */
  Uint S2MM_DMASR;             /* 0x00000034��  32      mixed   0x00010000      DMA Status register */
  Uint reserves3[4];           /* 38h - 44h Reserved N/A */
  Uint S2MM_DA;                /* 0x00000048    32      rw      0x00000000      Destination Address. Lower 32 bit address.*/
  Uint S2MM_DA_MSB;            /* 0x0000004c    32      rw      0x00000000      Destination Address. Upper 32 bit address.*/
  Uint reserved4[2];           /* 50h - 54h Reserved N/A */
  Uint S2MM_LENGTH;            /* 0x00000058    32      rw      0x00000000      Buffer Length (Bytes) */

  /* Simple Mode */
  /* 0. MM2S_DMASR.Halted=0,Idle=1���ǧ */
  /* 1. Start the MM2S channel (MM2S_DMACR.RS = 1).*/
  /* 2. Write a valid source address to MM2S_SA+MM2S_SA_MSB register.*/
  /* 3. Write the bytes to MM2S_LENGTH register. A value of zero has no effect.*/

  /* 0. S2MM_DMASR.Halted=0,Idle=1���ǧ */
  /* 1. Start the S2MM channel (S2MM_DMACR.RS = 1).*/
  /* 2. Write a valid destination address to S2MM_DA+S2MM_DA_MSB register.*/
  /* 3. Write the bytes to S2MM_LENGTH register. A value of zero has no effect.*/

  /* 4. MM2S_DMASR.bit4-6!=0�ʤ饨�顼 */
  /* 4. S2MM_DMASR.bit4-6!=0�ʤ饨�顼 */
  /* 4. MM2S_DMASR.IOC_Irq��1�ˤʤ�ޤ��Ե�,1��񤤤�reset */
  /* 4. S2MM_DMASR.IOC_Irq��1�ˤʤ�ޤ��Ե�,1��񤤤�reset */
};

/* reg_ctrl */
enum { EXRING_IDLE, EXRING_BUSY};
enum { LMRING_IDLE, LMRING_BUSY};
enum { CMD_NOP, CMD_RESET, CMD_SCON, CMD_EXEC};
struct reg_ctrl {
  struct i0 {
    Ull  stat; /* +0000 bit15-12:LMM_SIZE, bit11-8:EMAX_DEPTH, bit7-4:LMRING, bit3-0:EXRING */
    Uint mcid; /* +0008 maximum chip-ID of IMAX (<EMAX_NCHIP) to be chained (activated) */
    Uint dmy0;
    Uint cmd;  /* +0010 host writes Ull cmd then chip# is propagated to succesors */
  /*Uint cid;*//* +0012 chip# ( set by write to cmd ) */
    Uint dmy1;
    Ull  dmy2;
    Ull  adtr; /* +0020 */
    Ull  dmy3;
    Ull  csel; /* +0030 */
    Ull  dmrp; /* +0038 DMAREAD-PREF */
    Ull  dmy4[1016];
    struct conf                    conf[AMAP_DEPTH][EMAX_WIDTH];  /* +2000-3fff */
    struct {Ull  br[UNIT_WIDTH];}  breg[AMAP_DEPTH][EMAX_WIDTH];  /* +4000-5fff *//* unit[cid][EMAX_DEPTH].breg[x][EMAX_WIDTH].br[UNIT_WIDTH] is used */
    struct {Uint ea0b ; /* ea0 base   (for avoiding ld-mask-st, should be extended to 32bits (lower 18bit is available)) */
      /*Ull  dmy0 :14;*/
        Uint ea0o ; /* ea0 offset (for avoiding ld-mask-st, should be extended to 32bits (lower 18bit is available)) */
      /*Ull  dmy1 :14;*/
        Uint ea1b ; /* ea1 base   (for avoiding ld-mask-st, should be extended to 32bits (lower 18bit is available)) */
      /*Ull  dmy2 :14;*/
        Uint ea1o ; /* ea1 offset (for avoiding ld-mask-st, should be extended to 32bits (lower 18bit is available)) */
      /*Ull  dmy3 :14;*/
        Uint top  ; /* LMM-top virtual-address */
      /*Ull  dmy4 : 1;*/
        Uint bot  ; /* LMM-bot virtual-address */
      /*Ull  dmy5 : 1;*/
        Ull  dmy6 ;}           addr[AMAP_DEPTH][EMAX_WIDTH];  /* +6000-7fff */
    struct {Ull  reg[UNIT_WIDTH];} lddmrw[AMAP_DEPTH][EMAX_WIDTH];/* +8000-9fff *//* lddmq/trans-r,lddmq-w */
    Ull dmy5[3072]; /* +a000-ffff */
  } i[EMAX_NCHIP]; /* 0000-ffff */
};

/* emax7 host control */
enum { STATUS_IDLE, STATUS_CONF, STATUS_SCON, STATUS_REGV, STATUS_RANGE, STATUS_DRAIN, STATUS_LOAD, STATUS_START, STATUS_EXEC, STATUS_TERM };

pthread_mutex_t axi_dma_mutex;

struct emax7 { /* host status of EMAX7 */
  volatile Ull   dma_ctrl;  /* struct dma_ctrl *dma_ctrl  DMA control */
  volatile Ull   reg_ctrl;  /* struct reg_ctrl *reg_ctrl  REG control */

  Ull   status            : 4;
  Ull   csel_save         : 2;
  Ull   last_conf            ; /* for insn_reuse */
  Ull   lmmic             : 1; /* 0:lmm[0] is curent, 1:lmm[1] is current */
  Ull   lmmio             : 1; /* 0:lmm[0] is prev,   1:lmm[1] is prev    */
  Ull   mapdist           : 6; /* specified mapdist */
  Ull   lastdist          : 6; /* lastdist for DYNAMIC_SCON */
  struct lmmi lmmi[EMAX_NCHIP][AMAP_DEPTH][EMAX_WIDTH][2]; /* lmmi for host (len/ofs/top are resolved) */
  Ull   lmmi_bitmap[EMAX_WIDTH];      /* based on lmmi[*][EMAX_WIDTH][2].v */
  Uchar lmmd[AMAP_DEPTH][EMAX_WIDTH]; /* chip#7,6,..,0:clean, 1:dirty, exec��store�ս��1, drainľ��0 */

#ifndef IGNORE_LMMI_BLKGATHER
  Ull   plist                ; /* pointer-list */
  Ull   blkcount          : 7; /* active block number */
  Ull   blksize           : 9; /* 1:64 2:128 3:256 dwords */
  Ull   lmmblktop            ; /* LMM-addr     for LDRQ(blk>0) */
  Ull   lmmblklen            ; /* total dwords for LDRQ(blk>0) */
#endif

  Ull   rw                    ; /* 0:load(mem->lmm), 1:store(lmm->mem)      */
  Ull   ddraddr               ; /* ddr-address                              */
  Ull   lmmaddr               ; /* lmm-address                              */
  Ull   dmalen                ; /* dma-length                               */
  Ull   sigwait               ; /* 0:no macropipe+sigwait, 1:macropipe+sigwait */
  int   *sigstat              ; /* ->th_args.stat (0:idle, 1:run, 2:wait)   */
  sigset_t *sigset            ; /* for sigmask/sigwait                      */

#ifndef IGNORE_LDDMQ_HANDSHAKE
  Ull   fsm_busy          : 1; /* for LDDMQ and TR */
  Ull   lmwd_valid        : 1; /* for LDDMQ */
  Ull   tcureg_valid      : 1; /* fsm->ARM   0 -> 1 -> 1 -> 0 -> 0 -> 0                              */
  Ull   tcureg_ready      : 1; /* fsm<-ARM   0 -> 0 -> 1 -> 0 -> 0 -> 0                              */
  Ull   tcureg_last       : 1; /* fsm->ARM   0 -> 0 -> 0 -> 1 -> 1 -> 0                              */
  Ull   tcureg_term       : 1; /* fsm<-ARM   0 -> 0 -> 0 -> 0 -> 1 -> 0                              */
  Ull   tcureg[UNIT_WIDTH]   ; /* tcu-data        of tcu                       v                     */
                               /* from ARM:  svc 0x1010 ... tcureg_valid->x0                         */
                               /* from ARM:  svc 0x1011 ... 1->tcureg_ready                          */
                               /* from ARM:  svc 0x1012 ... tcureg_last->x0                          */
                               /* from ARM:  svc 0x1013 ... 1->tcureg_term                           */
                               /* from ARM:  svc 0x1014 ... tcureg[3:0]->x3,2,1,0                    */
#endif
} emax7[EMAX_NLANE];

volatile struct emax_info {
  Ull  dma_phys;     // kern-phys
  Ull  dma_vadr;     // not used
  Ull  dma_mmap;     // user-virt Contiguous 64K register space
  Ull  reg_phys;     // kern-phys
  Ull  reg_vadr;     // not used
  Ull  reg_mmap;     // user-virt Contiguous 8GB space including LMM space
  Ull  lmm_phys;     // kern-phys
  Ull  lmm_vadr;     // not used
  Ull  lmm_mmap;     // user-virt Contiguous 4GB space for LMM space
  Ull  ddr_phys;     // kern-phys
  Ull  ddr_vadr;     // not used
  Ull  ddr_mmap;     // user-virt Contiguous 4GB space in DDR-high-6GB space
  int  driver_use_1;
  int  driver_use_2;
} emax_info[EMAX_NLANE];

/*  ... for ARMSIML only */
#define DMA_BASE2_PHYS	 0x50000000
#define REG_BASE2_PHYS	 0x50100000
#define REG_CONF_OFFS    0x00002000
#define REG_BREG_OFFS    0x00004000
#define REG_ADDR_OFFS    0x00006000
#define REG_LDDM_OFFS    0x00008000
#define REG_AREA_MASK    0x0000ffff
#define LMM_BASE2_PHYS 	 0x60000000
#define MEM_VALID_ADDR	 0xafffffff

#ifndef NO_EMAX7LIB_BODY

#ifdef ARMZYNQ
/*******************************************************************************/
/******************************** ZYNQ-COMMON **********************************/
/*******************************************************************************/

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <linux/ioctl.h>

#define DMA_BASE_PHYS	 0x00000000a4000000LL
#define DMA_BASE_PHYSOFS 0x0000000000010000LL
#define DMA_MMAP_SIZE	 0x0000000000010000LL
/*  ... 64KB  */
#define REG_BASE_PHYS	 0x0000020800000000LL
#define REG_BASE_PHYSOFS 0x0000000800000000LL
#define REG_MMAP_SIZE	 0x0000000200000000LL
/*  ... 8GB REGS(4G)+LMM(4G) */
#define LMM_BASE_PHYS	 0x0000020900000000LL
#define LMM_BASE_PHYSOFS 0x0000000800000000LL
/*  ... 4GB   */
#define DDR_BASE_PHYS	 0x0000050000000000LL
#define DDR_MMAP_SIZE	 0x0000000100000000LL
/*  ... 4GB   */

#define EMAX_IOC_MAGIC  60
/* Please use a different 8-bit number in your code */
#define EMAX_IORESET			_IO( EMAX_IOC_MAGIC, 0)
#define EMAX_GET_ACPMEM			_IOWR(EMAX_IOC_MAGIC,  1, unsigned long)
#define EMAX_IOC_MAXNR 2

static int filter(struct dirent *dir)
{
  return dir->d_name[0] == '.' ? 0 : 1;
}

static void trim(char *d_name)
{
  char *p = strchr(d_name, '\n');
  if (p != NULL) *p = '\0';
}

static int is_target_dev(char *d_name, char *target)
{
  char path[32];
  char name[32];
  FILE *fp;
  sprintf(path, "/sys/class/uio/%s/name", d_name);
  if ((fp = fopen(path, "r")) == NULL) return 0;
  if (fgets(name, sizeof(name), fp) == NULL) {
    fclose(fp);
    return 0;
  }
  fclose(fp);
  if (strcmp(name, target) != 0) return 0;
  return 1;
}

static int get_reg_size(char *d_name)
{
  char path[32];
  char size[32];
  FILE *fp;
  sprintf(path, "/sys/class/uio/%s/maps/map0/size", d_name);
  if ((fp = fopen(path, "r")) == NULL) return 0;
  if (fgets(size, sizeof(size), fp) == NULL) {
    fclose(fp);
    return 0;
  }
  fclose(fp);
  return strtoull(size, NULL, 16);
}

emax7_open(int NLANE)
/* HPM���ͳ��������쥸�����˥ꥻ�å����� */
/* HPP���ͳ�������������۶��֤˼��� */
/* ACP���ͳ����conf/lmmi/regv���֤��۶��֤˼��� */
{
  struct dirent **namelist;
  int  num_dirs, dir, uiolen;
  int  reg_size;
  char path[1024];
  int  fd_dma;
  int  fd_reg;
  int  fd_ddr;
  char *UIO_AXI_C2C       = "axi_chip2chip\n";
  char *UIO_AXI_MM2S      = "axi_mm2s_mapper\n";
  char *UIO_DMA           = "dma\n";
  char *UIO_AXI_EMAX6     = "emax6\n";
  char *UIO_DDR_HIGH      = "ddr_high\n";
  int  fd_dma_found = 0;
  int  emax7_found = 0;
  int  i;

  pthread_mutex_init(&axi_dma_mutex, NULL);

  if ((num_dirs = scandir("/sys/class/uio", &namelist, filter, alphasort)) == -1)
    return (NULL);

  for (dir = 0; dir < num_dirs; ++dir)
    trim(namelist[dir]->d_name);
 
  for (uiolen=4; uiolen<6; uiolen++) { /* /dev/uioX -> /dev/uio1X ... */
    for (dir = 0; dir < num_dirs; ++dir) {
      if (strlen(namelist[dir]->d_name)!=uiolen) /* ignore /dev/uio1X */
	continue;
      if (is_target_dev(namelist[dir]->d_name, UIO_AXI_EMAX6)) {
	sprintf(path, "/dev/%s", namelist[dir]->d_name);
	if ((fd_reg = open(path, O_RDWR | O_SYNC)) == -1) {
	  printf("open failed. %s", UIO_AXI_EMAX6);
	  return (NULL);
	}
	printf("%s: %s", path, UIO_AXI_EMAX6);
	if (emax7_found >= EMAX_NLANE || emax7_found >= NLANE) {
	  printf("emax7_found > EMAX_NLANE || emax7_found >= given_NLANE (skip)\n");
	  continue; /* skip rest of EMAX7 */
	}
	// mmap(cache-off) 4KB aligned
	emax_info[emax7_found].reg_phys = REG_BASE_PHYS+REG_BASE_PHYSOFS*emax7_found;
	emax_info[emax7_found].reg_mmap = (Ull)mmap(NULL, REG_MMAP_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd_reg, 0); /* 4GB */
	close(fd_reg);
	if (emax_info[emax7_found].reg_mmap == MAP_FAILED) {
	  printf("fd_reg mmap() failed. errno=%d\n", errno);
	  return (NULL);
	}
	emax_info[emax7_found].lmm_phys = LMM_BASE_PHYS+LMM_BASE_PHYSOFS*emax7_found;
	emax_info[emax7_found].lmm_mmap = emax_info[emax7_found].reg_mmap + (LMM_BASE_PHYS - REG_BASE_PHYS);
	emax7_found++;
      }
    }
  }
  for (uiolen=4; uiolen<6; uiolen++) {
     for (dir = 0; dir < num_dirs; ++dir) {
      if (strlen(namelist[dir]->d_name)!=uiolen) /* ignore /dev/uio1X */
	continue;
      if (is_target_dev(namelist[dir]->d_name, UIO_DMA) && (reg_size = get_reg_size(namelist[dir]->d_name))) {
	sprintf(path, "/dev/%s", namelist[dir]->d_name);
	if ((fd_dma = open(path, O_RDWR | O_SYNC)) == -1)
	  continue;
	printf("%s: %s", path, UIO_DMA);
	if (fd_dma_found >= EMAX_NLANE || fd_dma_found >= NLANE) {
	  printf("fd_dma_found > EMAX_NLANE || fd_dma_found > given_NLANE (skip)\n");
	  continue; /* skip rest of FDDMA */
	}
	emax_info[fd_dma_found].dma_phys = DMA_BASE_PHYS+DMA_BASE_PHYSOFS*fd_dma_found;
	emax_info[fd_dma_found].dma_mmap = (Ull)mmap(NULL, reg_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd_dma, 0);
	close(fd_dma);
	if (emax_info[fd_dma_found].dma_mmap == MAP_FAILED)
	  continue;
	fd_dma_found++;
      }
      else if (is_target_dev(namelist[dir]->d_name, UIO_AXI_C2C)) {
	sprintf(path, "/dev/%s", namelist[dir]->d_name);
	if ((fd_reg = open(path, O_RDWR | O_SYNC)) == -1) {
	  printf("open failed. %s", UIO_AXI_C2C);
	  return (NULL);
	}
	printf("%s: %s", path, UIO_AXI_C2C);
	if (emax7_found >= EMAX_NLANE || emax7_found >= NLANE) {
	  printf("emax7_found > EMAX_NLANE || emax7_found > given_NLANE (skip)\n");
	  continue; /* skip rest of EMAX7 */
	}
	// mmap(cache-off) 4KB aligned
	emax_info[emax7_found].reg_phys = REG_BASE_PHYS+REG_BASE_PHYSOFS*emax7_found;
	emax_info[emax7_found].reg_mmap = (Ull)mmap(NULL, REG_MMAP_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd_reg, 0); /* 4GB */
	close(fd_reg);
	if (emax_info[emax7_found].reg_mmap == MAP_FAILED) {
	  printf("fd_reg mmap() failed. errno=%d\n", errno);
	  return (NULL);
	}
	emax_info[emax7_found].lmm_phys = LMM_BASE_PHYS+LMM_BASE_PHYSOFS*emax7_found;
	emax_info[emax7_found].lmm_mmap = emax_info[emax7_found].reg_mmap + (LMM_BASE_PHYS - REG_BASE_PHYS);
	emax7_found++;
      }
      else if (is_target_dev(namelist[dir]->d_name, UIO_AXI_MM2S)) {
	sprintf(path, "/dev/%s", namelist[dir]->d_name);
	if ((fd_reg = open(path, O_RDWR | O_SYNC)) == -1) {
	  printf("open failed. %s", UIO_AXI_MM2S);
	  return (NULL);
	}
	printf("%s: %s", path, UIO_AXI_MM2S);
	if (emax7_found >= EMAX_NLANE || emax7_found >= NLANE) {
	  printf("emax7_found > EMAX_NLANE || emax7_found > given_NLANE (skip)\n");
	  continue; /* skip rest of EMAX7 */
	}
	// mmap(cache-off) 4KB aligned
	emax_info[emax7_found].reg_phys = REG_BASE_PHYS+REG_BASE_PHYSOFS*emax7_found;
	emax_info[emax7_found].reg_mmap = (Ull)mmap(NULL, REG_MMAP_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd_reg, 0); /* 4GB */
	close(fd_reg);
	if (emax_info[emax7_found].reg_mmap == MAP_FAILED) {
	  printf("fd_reg mmap() failed. errno=%d\n", errno);
	  return (NULL);
	}
	emax_info[emax7_found].lmm_phys = LMM_BASE_PHYS+LMM_BASE_PHYSOFS*emax7_found;
	emax_info[emax7_found].lmm_mmap = emax_info[emax7_found].reg_mmap + (LMM_BASE_PHYS - REG_BASE_PHYS);
	emax7_found++;
      }
#define USE_CACHE_IMAX_4G
#if !defined(USE_CACHE_IMAX_4G)
      /* /dev/uio for non-cacheable space */
      else if (is_target_dev(namelist[dir]->d_name, UIO_DDR_HIGH)) {
	sprintf(path, "/dev/%s", namelist[dir]->d_name);
	if ((fd_ddr = open(path, O_RDWR | O_SYNC)) == -1) {
	  printf("open failed. %s",UIO_DDR_HIGH);
	  return (NULL);
	}
	printf("%s: %s", path, UIO_DDR_HIGH);
	// mmap(cache-on)  4KB aligned
	emax_info[0].ddr_phys = DDR_BASE_PHYS;
	emax_info[0].ddr_mmap = (Ull)mmap(NULL, DDR_MMAP_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd_ddr, 0); /* 4GB */
	close(fd_ddr);
	if (emax_info[0].ddr_mmap == MAP_FAILED) {
	  printf("fd_ddr mmap() failed. errno=%d\n", errno);
	  return (NULL);
	}
      }
#endif
    }
  }

#if defined(USE_CACHE_IMAX_4G)
  /* /dev/mem for cacheable space */
//if ((fd_ddr = open("/dev/mem", O_RDWR | O_SYNC)) == -1) { /* cache-off OK slow */
  if ((fd_ddr = open("/dev/mem", O_RDWR         )) == -1) { /* cache-on  OK fast */
    printf("open failed. %s",UIO_DDR_HIGH);
    return (NULL);
  }
  printf("/dev/mem\n"); /* mmap(cache-on) 4KB aligned */
  emax_info[0].ddr_phys = DDR_BASE_PHYS;
  emax_info[0].ddr_mmap = (Ull)mmap(NULL, DDR_MMAP_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, fd_ddr, DDR_BASE_PHYS); /* 4GB */
  close(fd_ddr);
  if (emax_info[0].ddr_mmap == MAP_FAILED) {
    printf("fd_ddr mmap() failed. errno=%d\n", errno);
    return (NULL);
  }
#endif

  for (dir = 0; dir < num_dirs; ++dir)
    free(namelist[dir]);
  free(namelist);

  if (!emax7_found) {
    printf("EMAX not found: %s", UIO_AXI_EMAX6);
    exit(1);
  }
  if (fd_dma_found != emax7_found) {
    printf("Warning: fd_dma_found(%d) != emax7_found(%d)\n", fd_dma_found, emax7_found);
    if (fd_dma_found < emax7_found)
      emax7_found = fd_dma_found;
    else
      fd_dma_found = emax7_found;
  }

  for (i=0; i<fd_dma_found; i++) {
    ((struct dma_ctrl*)emax_info[i].dma_mmap)->MM2S_DMACR = 0x00010004;
    ((struct dma_ctrl*)emax_info[i].dma_mmap)->MM2S_DMASR = 0x00017000;
    ((struct dma_ctrl*)emax_info[i].dma_mmap)->S2MM_DMACR = 0x00010004;
    ((struct dma_ctrl*)emax_info[i].dma_mmap)->S2MM_DMASR = 0x00017000;
  }
  for (i=1; i<emax7_found; i++) {
    emax_info[i].ddr_phys = emax_info[0].ddr_phys;
    emax_info[i].ddr_mmap = emax_info[0].ddr_mmap;
  }

  return (emax7_found);
}
#endif

/*******************************************************************************/
/******************************** Timer ****************************************/
/*******************************************************************************/

Ull nanosec_sav[EMAX_NLANE];
Ull nanosec[EMAX_NLANE][NANOS_CLASS];

sleep_nanosec(int nano)
{
#if defined(ARMSIML)
#else
  struct timespec ts;
  ts.tv_sec = 0;
  ts.tv_nsec = nano;
  nanosleep(NULL, &ts);
#endif
}

reset_nanosec(int LANE)
{
  int i;
  for (i=0; i<NANOS_CLASS; i++)
    nanosec[LANE][i] = 0;
#if defined(ARMSIML)
  nanosec_sav[LANE] = _getclk(0);
#else
  struct timespec ts;
  clock_gettime(0, &ts); /*CLOCK_REALTIME*/
  nanosec_sav[LANE] = 1000000000*ts.tv_sec + ts.tv_nsec;
#endif
}

get_nanosec(int LANE, int class_no)
{
  Ull nanosec_now;
#if defined(ARMSIML)
  nanosec_now = _getclk(0);
  nanosec[LANE][class] += nanosec_now - nanosec_sav[LANE];
  nanosec[LANE][NANOS_TOTAL] += nanosec_now - nanosec_sav[LANE];
  nanosec_sav[LANE] = nanosec_now;
#else
  struct timespec ts;
  clock_gettime(0, &ts); /*CLOCK_REALTIME*/
  nanosec_now = 1000000000*ts.tv_sec + ts.tv_nsec;
  nanosec[LANE][class_no] += nanosec_now - nanosec_sav[LANE];
  nanosec[LANE][NANOS_TOTAL] += nanosec_now - nanosec_sav[LANE];
  nanosec_sav[LANE] = nanosec_now;
#endif
}

show_nanosec(int LANE)
{
#if defined(ARMSIML)
  printf("LANE%d SIML_cycle/1000: ARM:%d DRAIN:%d CONF:%d REGV:%d RANGE:%d LOAD:%d EXEC:%d total:%d\n",
	 LANE,
	 (Uint)(nanosec[LANE][NANOS_ARM]/1000),
	 (Uint)(nanosec[LANE][NANOS_DRAIN]/1000),
	 (Uint)(nanosec[LANE][NANOS_CONF]/1000),
	 (Uint)(nanosec[LANE][NANOS_REGV]/1000),
	 (Uint)(nanosec[LANE][NANOS_RANGE]/1000),
	 (Uint)(nanosec[LANE][NANOS_LOAD]/1000),
	 (Uint)(nanosec[LANE][NANOS_EXEC]/1000),
	 (Uint)(nanosec[LANE][NANOS_TOTAL]/1000));
#else
  printf("LANE%d usec: ARM:%d DRAIN:%d CONF:%d REGV:%d RANGE:%d LOAD:%d EXEC:%d total:%d\n",
	 LANE,
	 (Uint)(nanosec[LANE][NANOS_ARM]/1000),
	 (Uint)(nanosec[LANE][NANOS_DRAIN]/1000),
	 (Uint)(nanosec[LANE][NANOS_CONF]/1000),
	 (Uint)(nanosec[LANE][NANOS_REGV]/1000),
	 (Uint)(nanosec[LANE][NANOS_RANGE]/1000),
	 (Uint)(nanosec[LANE][NANOS_LOAD]/1000),
	 (Uint)(nanosec[LANE][NANOS_EXEC]/1000),
	 (Uint)(nanosec[LANE][NANOS_TOTAL]/1000));
#endif
}

/*******************************************************************************/
/******************************** DMA-START ************************************/
/*******************************************************************************/

#if !defined(EMAXNC)
emax7_check_lmmi_and_dma(int LANE, int mode, int phase, int lastdist, int c, int i, int j)
{
  /* mode   0:array, 1:drain */
  /* phase  0:nop,   1:drain, 2:load, 3exec */
  /* lastdist */
  /* i      row              */
  /* j      col              */
  /* lmmi������˸ƤФ��. lmd��Ϣ��lmd���֤����lmr/lmx�򸡺�(+lastdist����ͳ) */
  /*                   ����lmd    ���֤�,"lmmi[i         ][lmmic]" */
  /* lastdist=>0�ξ��,����lmw/lmx���֤�,"lmmi[i+lastdist][lmmio]" */
  /* ����,lmd�ξ��,SCON���Ƥ⤷�ʤ��Ƥ�EXEC��Ʊ����DRAIN (lastdist=0�ξ���lmm��Ⱦʬ��lmd/lmw�˻Ȥ�ʬ��������) */
  /* lastdist=0�ξ��,DYNAMIC_SCON��̵��̣�ʤΤǽ����̤� */
  /* lastdist>0�ξ��,DYNAMIC_SCON����SCONͭ��Ͻ����̤� */
  /*                 SCON̵��ξ��,���⤽��lmd���֤����ʤ��Τ�lmd�����̵�뤹�٤�.��lmw/lmx��EXEC��DRAIN��ɬ�� */
  /*                 ������������,lmd��Ȥ��������Ǥ�DYNAMIC_SCON��Ȥ�ʤ��Ϥ��ʤΤ�,�����б����ʤ� */
  int k, m = (i+lastdist)%EMAX_DEPTH; /* lmmo-index */
  int lmmc_topz;
  int lmmc_ofsz;
  int lmmo_stat;
  int lmmc_stat;
  int lmm_ready;
  int lmm_readz;
  int mark;

  struct lmmi *lmmiop  = &emax7[LANE].lmmi[c][m][j][emax7[LANE].lmmio];
  struct lmmi *lmmicp  = &emax7[LANE].lmmi[c][i][j][emax7[LANE].lmmic];
  struct lmmi *lmmiop1 = &emax7[LANE].lmmi[c][(m+1)%EMAX_DEPTH][j][emax7[LANE].lmmio];
  struct lmmi *lmmicp1 = &emax7[LANE].lmmi[c][(i+1)%EMAX_DEPTH][j][emax7[LANE].lmmic];

  Ull dmadr;
  int dmlen;
  Ull dmnxt;
  int dmrw; /* 0:mem->lmm 1:lmm->mem */
  static Ull concat_adr[EMAX_NLANE][EMAX_NCHIP]; /* NULL:invalid, !NULL:top_addr */
  static int concat_len[EMAX_NLANE][EMAX_NCHIP]; /* byte-len */

  /* check_lmmi */
  if ((phase == 1 && mode == 0) || phase == 2 || phase == 3) { /* (drain && array) || load || exec */
    lmmc_topz = (lmmicp->top == 0);
    lmmc_ofsz = (lmmicp->ofs == 0);
    lmmo_stat = (lmmiop->v<<3)|(lmmiop->rw<<2)|(lmmiop->f<<1)|(lmmiop->p); /* v|rw|f|p */
    lmmc_stat =((lmmicp->v & ~lmmicp->hcopy & ~lmmicp->vcopy & ((lmmicp->f&lmmicp->p) | !lmmc_topz))<<3)|(lmmicp->rw<<2)|(lmmicp->f<<1)|(lmmicp->p); /* v= ~copy & (OP_LDDMQ/OP_TR �ޤ��� ptop!=NULL) */
    lmm_ready = (lmmiop->v && lmmiop->blk == lmmicp->blk && lmmiop->len == lmmicp->len && lmmiop->top == lmmicp->top);
    lmm_readz = (lmmiop->v && lmmiop->blk == lmmicp->blk && lmmiop->len == lmmicp->len &&(lmmiop->top+(Sll)(int)lmmiop->ofs) == lmmicp->top);
  }

  /* lmx: bitmap�򸡺���,��addr+len�ȼ�addr�����,Ϣ³�ʤ�Ϣ�뤷����addr/len����¸.�ǽ��ޤ�����Ϣ³�ʤ���¸addr/len�ޤ��ϸ�addr/len��Ȥä�DMA */

#if 0
if (mode==0) {
printf("EX..DMA phase=%x i=%d m=%d j=%d lmmic/o=%x/%x lmmc_stat=%x(dirty=%x) lmmo_stat=%x(dirty=%x)\n", phase, i, m, j, emax6.lmmic, emax6.lmmio, lmmc_stat, emax6.lmmd[i][j], lmmo_stat, emax6.lmmd[m][j]);
}
else {
printf("DR..DMA phase=%x i=%d j=%d dirty=%x\n", phase, i, j, emax6.lmmd[i][j]);
}
#endif

  /*     LD/f=0 ��A   | LD/f=0 reuse A | LD/f=0 reuse A | LD/f=0     ��B | LD/f=0 reuse B | LD/f=0 reuse B */
  /*     LD/f=0 ��A   | LD/f=0 reuse A | LD/f=1     ��A | LD/f=0     ��B | LD/f=0 reuse B | LD/f=1     ��B */
  /*     ST/f=0   A   | ST/f=0 ��DR  A | ST/f=0 ��DR  A | ST/f=0 A��   B | ST/f=0 ��DR  B | ST/f=0 ��DR  B */
  /*���� ST/f=1 ��A   | ST/f=1 ��DR  A | ST/f=1 ��DR  A | ST/f=1 A�� ��B | ST/f=1 ��DR  B | ST/f=1 ��DR  B */
  /*���� ST/f=1 ��A   | ST/f=1 ��DR��A | ST/f=0 ��DR  A | ST/f=1 A�� ��B | ST/f=1 ��DR��B | ST/f=0 ��DR  B */
  /*                  |��st����A����� |                |                |��st����B����� |                */
  /*���� ST/f=1 ��A   | ST/f=0 ��DR  A | ST/f=0 ��DR  A | ST/f=1 A�� ��B | ST/f=0 ��DR  B | ST/f=0 ��DR  B */
  /*���� ST/f=1 ��A   |           A!=B�ξ�缫ưdrainͭ | ST/f=1 A�� ��B                                   */
  /*���� ST/f=1 ��A   |           A==B�ξ�缫ưdrain̵ | ST/f=1 ��DR��B                                   */
  /*                  |                                 |��st����B�����                                   */
  /*���� ST/f=1 ��A   |           A==B�ξ���ưdrain A | ST/f=1     ��B                                   */

  /*���� ST/f=1 ��A   | ST/f=1 A�� ��A | ST/f=0 ��DR  A | ST/f=1 A�� ��B | ST/f=1 B�� ��B | ST/f=0 ��DR  B */
  /*���� ST/f=1 ��A   | ST/f=0 ��DR  A | ST/f=0 ��DR  A | ST/f=1 A�� ��B | ST/f=0 ��DR  B | ST/f=0 ��DR  B */
  /*���� ST/f=1 ��A   |           A!=B�ξ�缫ưdrainͭ | ST/f=1 A�� ��B                                   */
  /*���� ST/f=1 ��A   |           A==B�ξ�缫ưdrainͭ | ST/f=1 A�� ��B                                   */

  if      (phase == 1) { /* drain */
    if      (mode==0 && lmmo_stat==12 && !lmm_ready && lmmc_stat!=13 && (emax7[LANE].lmmd[m][j]&1<<c)) { mark=1;emax7[LANE].lmmd[m][j]&=~(1<<c);dmadr=lmmiop->top;dmlen=lmmiop->len;dmnxt=lmmiop1->top;dmrw=1;}/* ��2 lmw&!lmd drain */
    else if (mode==0 && lmmo_stat==14 /*&& !lmm_ready*/              && (emax7[LANE].lmmd[m][j]&1<<c)) { mark=1;emax7[LANE].lmmd[m][j]&=~(1<<c);dmadr=lmmiop->top;dmlen=lmmiop->len;dmnxt=lmmiop1->top;dmrw=1;}/* ��4 lmx      drain */
    else if (mode==1 &&                                                 (emax7[LANE].lmmd[i][j]&1<<c)) { mark=1;emax7[LANE].lmmd[i][j]&=~(1<<c);dmadr=lmmicp->top;dmlen=lmmicp->len;dmnxt=lmmicp1->top;dmrw=1;}/* �� drain_dirty_lmm */
    else                                                                                               { mark=0;                                                                                              }
  }
  else if (phase == 2) { /* load */
    if     ((lmmc_stat== 8            && !lmm_ready)                                                                                                                                                           /* ��1 lmr & !ready */
         || (lmmc_stat== 9            && !lmm_readz)                                                                                                                                                           /* ��7 lmp & !readz */
         || (lmmc_stat==10                         )                                                                                                                                                           /* ��3 lmf always load */
         || (lmmc_stat==14                         ))                                                  { mark=1;                                dmadr=lmmicp->top;dmlen=lmmicp->len;dmnxt=lmmicp1->top;dmrw=0;}/* ��3 lmx always load */
    else                                                                                               { mark=0;                                                                                              }/* skip load */
  }
  else if (phase == 3) { /* exec */
    if      (lmmc_stat== 9 && (lastdist||!lmmc_ofsz)) { mark=1;                                                                                 dmadr=lmmicp->top;dmlen=lmmicp->len;                   dmrw=0;}/* ��5 lmp */
    else if (lmmc_stat==12 || lmmc_stat==14         ) { mark=0;          emax7[LANE].lmmd[i][j]|=(1<<c);                                                                                                      }/* ��6 lmw/lmx */
    else if (lmmc_stat==13                          ) { mark=            emax7[LANE].lmmd[m][j]& (1<<c);                  emax7[LANE].lmmd[m][j]|=((!lastdist)<<c);dmadr=lmmicp->top;dmlen=lmmicp->len;dmrw=1;}/* ��6 lmd & dirty */
#ifndef IGNORE_LDDMQ_HANDSHAKE
//  else if (lmmc_stat==11                          ) { mark=1;        } /* LDDMQ */
//  else if (lmmc_stat==15                          ) { mark=1;        } /* TR    */
#endif
    else                                              { mark=0;        } /* skip pdrain/pload */
  }

  if (mark) {
#if 1
    if (phase == 1) { /* drain */
      /* concat_adr=0        adr0,L=0        | adr1,L=0        | adr2,L=0        */
      /* concat_adr=adr0,L=0 adr0,L=0,mark=0 | adr1,L=0        | adr2,L=0        */
      /* concat_adr=adr0,L=1          mark=0 | adr1,L=0,mark=0 | adr2,L=0        */
      /* concat_adr=adr0,L=2          mark=0 |          mark=0 | adr2,L=0,mark=1 */
    //printf("drain %d.%d: adr=%08.8x len=%08.8x nxt=%08.8x\n", i, j, (Uint)dmadr, (Uint)dmlen, (Uint)dmnxt);
      if ((emax7[LANE].lmmd[(m+1)%EMAX_DEPTH][j]&(1<<c)) && (dmadr+(dmlen+1)*sizeof(Uint)) == dmnxt) {
	if (!concat_adr[LANE][c]) { concat_adr[LANE][c] = dmadr; concat_len[LANE][c] = dmlen; }
	else                      { concat_len[LANE][c] += dmlen+1; }
	if (concat_len[LANE][c] < 8192) mark = 0;
      }
      else {
	if (concat_adr[LANE][c])  { concat_len[LANE][c] += dmlen+1; }
      }
    }
    else if (phase == 2) { /* load */
    //printf("load %d.%d: adr=%08.8x len=%08.8x nxt=%08.8x\n", i, j, (Uint)dmadr, (Uint)dmlen, (Uint)dmnxt);
      if (lmmicp1->v && (dmadr+(dmlen+1)*sizeof(Uint)) == dmnxt) {
	if (!concat_adr[LANE][c]) { concat_adr[LANE][c] = dmadr; concat_len[LANE][c] = dmlen; }
	else                      { concat_len[LANE][c] += dmlen+1; }
	if (concat_len[LANE][c] < 8192) mark = 0;
      }
      else {
	if (concat_adr[LANE][c])  { concat_len[LANE][c] += dmlen+1; }
      }
    }
#endif
  }

  /* dma */
  if (mark) {
    emax7[LANE].rw = dmrw;
    if (phase == 1) { /* drain */
      emax7[LANE].ddraddr = (concat_adr[LANE][c])?concat_adr[LANE][c]:dmadr; /* address should be 4B-aligned */
      emax7[LANE].lmmaddr = emax7[LANE].ddraddr;
      emax7[LANE].dmalen  = (concat_adr[LANE][c])?concat_len[LANE][c]:dmlen; /* length should be # of words */
    }
    else if (phase == 3 && dmrw==1) { /* pdrain */
      emax7[LANE].ddraddr = dmadr+(Sll)(int)lmmicp->ofs; /* ������PDRAIN address should be 4B-aligned */
      emax7[LANE].lmmaddr = emax7[LANE].ddraddr;
      emax7[LANE].dmalen  = dmlen; /* length should be # of words */
    }
    else if (phase == 2                /* load */
	  ||(phase == 3 && dmrw==0)) { /* pload *//* address should be 4B-aligned *//* length should be # of words */
      if (lmmicp->blk==0) { /* inf */
	if (phase == 2) { /* load */
	  emax7[LANE].ddraddr = (concat_adr[LANE][c])?concat_adr[LANE][c]:dmadr; /* address should be 4B-aligned */
	  emax7[LANE].lmmaddr = emax7[LANE].ddraddr;
	  emax7[LANE].dmalen  = (concat_adr[LANE][c])?concat_len[LANE][c]:dmlen; /* length should be # of words */
	}
	else {
	  emax7[LANE].ddraddr = dmadr+(Sll)(int)lmmicp->ofs; /* ������PLOAD address should be 4B-aligned */
	  emax7[LANE].lmmaddr = emax7[LANE].ddraddr;
	  emax7[LANE].dmalen  = dmlen; /* length should be # of words */
	}
#ifndef IGNORE_LMMI_BLKGATHER
	emax7[LANE].blksize    = 0; /* max:10bit */
#endif
      }
#ifndef IGNORE_LMMI_BLKGATHER
      else { /* 16,32,64 */
	if (phase == 2) /* load */
	  emax7[LANE].plist = dmadr+emax7[LANE].blkcount*8; /* address should be 4B-aligned */
	else
	  emax7[LANE].plist = dmadr+emax7[LANE].blkcount*8+(Sll)(int)lmmicp->ofs; /* ������PLOAD address should be 4B-aligned */
	emax7[LANE].blksize  = 32<<lmmicp->blk; /* max:10bit */
	if (emax7[LANE].blkcount==0) {
	  emax7[LANE].lmmblktop = 0; /* ������̤���������� ��Ƭ���ɥ쥹��0�ʤΤ�,addr_range�˹��פ�ɬ�� */
	  emax7[LANE].lmmblklen = dmlen; /* length should be # of words */
	}
	emax7[LANE].ddraddr    = emax7[LANE].plist; /* address should be 4B-aligned */
	emax7[LANE].lmmaddr    = emax7[LANE].lmmblktop;
	emax7[LANE].dmalen     = (emax7[LANE].lmmblklen<emax7[LANE].blksize)?emax7[LANE].lmmblklen:emax7[LANE].blksize-1;
	emax7[LANE].lmmblktop += emax7[LANE].blksize*sizeof(Ull);
	emax7[LANE].lmmblklen = (emax7[LANE].lmmblklen<emax7[LANE].blksize)?0:(emax7[LANE].lmmblklen-emax7[LANE].blksize);
	if (emax7[LANE].lmmblklen==0)
	  emax7[LANE].blkcount = 0;
	else
	  emax7[LANE].blkcount++; /* ������̤���������� continue ʣ�����DMA��ư��ɬ�� */
      }
#endif
    }
#if 0
if (mode==0) {
printf("EX==DMA phase=%x i=%d m=%d j=%d lmmic/o=%x/%x lmmc_stat=%x(dirty=%x) lmmo_stat=%x(dirty=%x) mark=%x", phase, i, m, j, emax6.lmmic, emax6.lmmio, lmmc_stat, emax6.lmmd[i][j], lmmo_stat, emax6.lmmd[m][j], mark);
printf(" rw=0x%x ddraddr=%08.8x lmmaddr=%08.8x dmalen=0x%x\n", emax6.rw, (Uint)emax6.ddraddr, (Uint)emax6.lmmaddr, (Uint)emax6.dmalen);
}
else {
printf("DR==DMA phase=%x i=%d m=%d j=%d dirty=%x mark=%x", phase, i, m, j, emax6.lmmic, emax6.lmmio, emax6.lmmd[i][j], mark);
printf(" rw=0x%x ddraddr=%08.8x lmmaddr=%08.8x dmalen=0x%x\n", emax6.rw, (Uint)emax6.ddraddr, (Uint)emax6.lmmaddr, (Uint)emax6.dmalen);
}
#endif
    concat_adr[LANE][c] = 0;
    //pthread_mutex_lock(&axi_dma_mutex);
    emax7_kick_dma(LANE, j);
    //pthread_mutex_unlock(&axi_dma_mutex);
  }
}

emax7_sigwait(int LANE)
{
  /* If activated for DMA/EXEC, the speed is too slow due to many syscall */
  /* Only enq/deq should be managed by sigwait(). 20231218 Nakashima */
  int signo;
  if (emax7[LANE].sigwait) {
    *emax7[LANE].sigstat = 2; /* wait */
    sigwait(emax7[LANE].sigset, &signo);
    *emax7[LANE].sigstat = 1; /* run */
  }
}

#if defined(FPDDMA)
#define FPDDMA_DEFINED 1
static inline Ull arm64_read_dcache_line_size(void)
{
    Ull       ctr;
    Ull       dcache_line_size;
    const Ull bytes_per_word = 4;
    asm volatile ("mrs %0, ctr_el0" : "=r"(ctr) : : );
    asm volatile ("nop" : : : );
    dcache_line_size = (ctr >> 16) & 0xF;
    return (bytes_per_word << dcache_line_size);
}
static inline void arm64_flush_dcache_area(void* start, size_t size)
{
    Ull   vaddr           = (Ull)start;
    Ull   __end           = (Ull)start + size;
    Ull   cache_line_size = arm64_read_dcache_line_size();
    Ull   cache_line_mask = cache_line_size - 1;
    vaddr &= ~cache_line_mask;
    while (vaddr < __end) {
        asm volatile ("dc cvac, %0"  :  : "r"(vaddr) : );
        vaddr += cache_line_size;
    }
    asm volatile ("dsb	sy"  :  :  : );
}
static inline void arm64_flush_inv_dcache_area(void* start, size_t size)
{
    Ull   vaddr           = (Ull)start;
    Ull   __end           = (Ull)start + size;
    Ull   cache_line_size = arm64_read_dcache_line_size();
    Ull   cache_line_mask = cache_line_size - 1;
    vaddr &= ~cache_line_mask;
    while (vaddr < __end) {
        asm volatile ("dc civac, %0"  :  : "r"(vaddr) : );
        vaddr += cache_line_size;
    }
    asm volatile ("dsb	sy"  :  :  : );
}
#else
#define FPDDMA_DEFINED 0
#endif

emax7_kick_dma(int LANE, int j) /* col */
{
  int status_mm2s, status_s2mm;
  Ull dst, src;
  Uint pio_words, pio_loop, pio_i, pio_b4, pio_b8, pio_b16, pio_e4, pio_e8, pio_e16;

  if (!emax7[LANE].ddraddr)
    return (0);

  if (j != emax7[LANE].csel_save) {
    ((struct reg_ctrl*)emax7[LANE].reg_ctrl)->i[0].csel = j; /* DMA/LDDMQ/TRANS�Ѥ��о�col���å� */
    emax7[LANE].csel_save = j;
  }
  if (FPDDMA_DEFINED && emax7[LANE].dmalen > 1) { /* 4B/8B: ->PIO */
    /* kick dma_ctrl (Simple Mode) */
    if (emax7[LANE].rw == 0) { /* mem->lmm */
      ((struct dma_ctrl*)emax7[LANE].dma_ctrl)->MM2S_DMACR = 0x00010001;
#if defined(USE_CACHE_IMAX_4G)
      /* flush (keep valid) cache for ddraddr */
      arm64_flush_dcache_area(emax7[LANE].ddraddr, (emax7[LANE].dmalen+1)*sizeof(Uint));
#endif
      *(Ull*)&(((struct dma_ctrl*)emax7[LANE].dma_ctrl)->MM2S_SA) = emax7[LANE].ddraddr-emax_info[LANE].ddr_mmap+emax_info[LANE].ddr_phys; /* address should be 4B-aligned */
      ((struct dma_ctrl*)emax7[LANE].dma_ctrl)->MM2S_LENGTH = (emax7[LANE].dmalen+1)*sizeof(Uint);                                         /* length should be # of words */
      ((struct dma_ctrl*)emax7[LANE].dma_ctrl)->S2MM_DMACR = 0x00010001;
      *(Ull*)&(((struct dma_ctrl*)emax7[LANE].dma_ctrl)->S2MM_DA) = emax7[LANE].lmmaddr-emax_info[LANE].ddr_mmap+emax_info[LANE].lmm_phys; /* (emax7[LANE].awaddr & ~(sizeof(Ull)*UNIT_WIDTH-1)) */
      ((struct dma_ctrl*)emax7[LANE].dma_ctrl)->S2MM_LENGTH = (emax7[LANE].dmalen+1)*sizeof(Uint);                                         /* length should be # of words */
      do {
	/*emax7_sigwait(LANE);*/
	status_mm2s = ((struct dma_ctrl*)emax7[LANE].dma_ctrl)->MM2S_DMASR;
	status_s2mm = ((struct dma_ctrl*)emax7[LANE].dma_ctrl)->S2MM_DMASR;
	if ((status_mm2s & 0x71) || (status_s2mm & 0x71)) {
	  ((struct dma_ctrl*)emax7[LANE].dma_ctrl)->MM2S_DMACR = 0x00010004; /* reset */
	  printf("emax7_check_lmmi_and_dma(): mem->lmm status_mm2s=%08x, status_s2mm=%8.8x (malfunction)\n", status_mm2s, status_s2mm);
	  printf("emax7_check_lmmi_and_dma(): emax7[%d].ddraddr=%16.16x -> emax7[%d].lmmaddr=%16.16x len=%16.16x\n", LANE, emax7[LANE].ddraddr, LANE, emax7[LANE].lmmaddr, (emax7[LANE].dmalen+1)*sizeof(Uint));
	  break;
	}
      } while (!(status_mm2s & 0x2) || !(status_s2mm & 0x2));
      ((struct dma_ctrl*)emax7[LANE].dma_ctrl)->MM2S_DMASR = 0x00001000; /* clear */
    }
    else { /* lmm->mem */
      while (((struct reg_ctrl*)emax7[LANE].reg_ctrl)->i[0].stat & 0xffff00f0); //LMRING_BUSY
      ((struct reg_ctrl*)emax7[LANE].reg_ctrl)->i[0].dmrp = (1LL<<63)|((emax7[LANE].dmalen+1)*sizeof(Uint)<<40)|(emax7[LANE].lmmaddr-emax_info[LANE].ddr_mmap+emax_info[LANE].lmm_phys);
      /*printf("dmrp=%08.8x_%08.8x\n", (Uint)((((struct reg_ctrl*)emax7[LANE].reg_ctrl)->i[0].dmrp)>>32), (Uint)(((struct reg_ctrl*)emax7[LANE].reg_ctrl)->i[0].dmrp));*/
#if defined(USE_CACHE_IMAX_4G)
      /* invalidate cache for ddraddr */
      arm64_flush_inv_dcache_area(emax7[LANE].ddraddr, (emax7[LANE].dmalen+1)*sizeof(Uint));
#endif
      ((struct dma_ctrl*)emax7[LANE].dma_ctrl)->MM2S_DMACR = 0x00010001;
      *(Ull*)&(((struct dma_ctrl*)emax7[LANE].dma_ctrl)->MM2S_SA) = emax7[LANE].lmmaddr-emax_info[LANE].ddr_mmap+emax_info[LANE].lmm_phys; /* (emax7[LANE].awaddr & ~(sizeof(Ull)*UNIT_WIDTH-1)) */
      ((struct dma_ctrl*)emax7[LANE].dma_ctrl)->MM2S_LENGTH = (emax7[LANE].dmalen+1)*sizeof(Uint);                                         /* length should be # of words */
      ((struct dma_ctrl*)emax7[LANE].dma_ctrl)->S2MM_DMACR = 0x00010001;
      *(Ull*)&(((struct dma_ctrl*)emax7[LANE].dma_ctrl)->S2MM_DA) = emax7[LANE].ddraddr-emax_info[LANE].ddr_mmap+emax_info[LANE].ddr_phys; /* address should be 4B-aligned */
      ((struct dma_ctrl*)emax7[LANE].dma_ctrl)->S2MM_LENGTH = (emax7[LANE].dmalen+1)*sizeof(Uint);                                         /* length should be # of words */
      do {
	/*emax7_sigwait(LANE);*/
	status_mm2s = ((struct dma_ctrl*)emax7[LANE].dma_ctrl)->MM2S_DMASR;
	status_s2mm = ((struct dma_ctrl*)emax7[LANE].dma_ctrl)->S2MM_DMASR;
	if ((status_mm2s & 0x71) || (status_s2mm & 0x71)) {
	  ((struct dma_ctrl*)emax7[LANE].dma_ctrl)->MM2S_DMACR = 0x00010004; /* reset */
	  printf("emax7_check_lmmi_and_dma(): lmm->mem status_mm2s=%08x, status_s2mm=%8.8x (malfunction)\n", status_mm2s, status_s2mm);
	  printf("emax7_check_lmmi_and_dma(): emax7[%d].lmmaddr=%16.16x -> emax7[%d].ddraddr=%16.16x len=%16.16x\n", LANE, emax7[LANE].lmmaddr, LANE, emax7[LANE].ddraddr, (emax7[LANE].dmalen+1)*sizeof(Uint));
	  break;
	}
      } while (!(status_mm2s & 0x2) || !(status_s2mm & 0x2));
      /* end of DMAREADBUF */
      ((struct dma_ctrl*)emax7[LANE].dma_ctrl)->MM2S_DMASR = 0x00001000; /* clear */
      ((struct reg_ctrl*)emax7[LANE].reg_ctrl)->i[0].dmrp  = (0LL<<63); /* off */
    }
  }
  else { /* ���굡�¹Ԥˤ��cache-fill��ư���Ƥ��ޤ�.�굡���Ը��DMA��ư��,cache-fill�⤵���Τ�mismatch�Ȥʤ�� */
    /*printf("emax_info[LANE].lmm_mmap-emax_info[LANE].ddr_mmap=%08.8x_%08.8x\n", (Uint)((emax_info[LANE].lmm_mmap-emax_info[LANE].ddr_mmap)>>32), (Uint)(emax_info[LANE].lmm_mmap-emax_info[LANE].ddr_mmap));*/
    if (emax7[LANE].rw == 0) { /* mem->lmm */
      dst = emax7[LANE].lmmaddr-emax_info[LANE].ddr_mmap+emax_info[LANE].lmm_mmap;
      src = emax7[LANE].ddraddr;
#if 0
      printf("emax7[LANE].lmmaddr:%08.8x_%08.8x <- emax7[LANE].ddraddr:%08.8x_%08.8x\n",
	     (Uint)((Ull)((Ull*)emax7[LANE].lmmaddr)>>32), (Uint)(Ull)((Ull*)emax7[LANE].lmmaddr),
	     (Uint)((Ull)((Ull*)emax7[LANE].ddraddr)>>32), (Uint)(Ull)((Ull*)emax7[LANE].ddraddr));
#endif
    }
    else { /* lmm->mem */
      dst = emax7[LANE].ddraddr;
      src = emax7[LANE].lmmaddr-emax_info[LANE].ddr_mmap+emax_info[LANE].lmm_mmap;
#if 0
      printf("emax7[LANE].lmmaddr:%08.8x_%08.8x -> emax7[LANE].ddraddr:%08.8x_%08.8x\n",
	     (Uint)((Ull)((Ull*)emax7[LANE].lmmaddr)>>32), (Uint)(Ull)((Ull*)emax7[LANE].lmmaddr),
	     (Uint)((Ull)((Ull*)emax7[LANE].ddraddr)>>32), (Uint)(Ull)((Ull*)emax7[LANE].ddraddr));
#endif
    }
    /* src��dst��32B���饤�󤵤�Ƥ���,��¦�Τ��󥢥饤��ˤʤ뤳�ȤϤʤ� */
    pio_words = emax7[LANE].dmalen+1;
    if (src & (sizeof(Ull)-1) & sizeof(Uint)) { /* 4B-access 0,4 */
      *(Uint*)dst = *(Uint*)src;
      src += sizeof(Uint);
      dst += sizeof(Uint);
      pio_words--;
    }
    if (pio_words >= 2) {
      if (src & (sizeof(Dll)-1) & sizeof(Ull)) { /* 8B-access 0,4 */
	*(Ull*)dst = *(Ull*)src;
	src += sizeof(Ull);
	dst += sizeof(Ull);
	pio_words-=2;
      }
    }
    if (pio_words >= 4) {
      if (pio_loop = pio_words/(sizeof(Dll)/sizeof(Uint))) {
	for(pio_i=0; pio_i<pio_loop; pio_i++)
	  *((Dll*)dst + pio_i) = *((Dll*)src + pio_i);
	pio_words -= pio_loop*(sizeof(Dll)/sizeof(Uint));
	src += pio_loop*sizeof(Dll);
	dst += pio_loop*sizeof(Dll);
      }
    }
    if (pio_words >= 2) { /* 8B-access */
      *(Ull*)dst = *(Ull*)src;
      src += sizeof(Ull);
      dst += sizeof(Ull);
      pio_words-=2;
    }
    if (pio_words >= 1) { /* 4B-access */
      *(Uint*)dst = *(Uint*)src;
      src += sizeof(Uint);
      dst += sizeof(Uint);
      pio_words--;
    }
  }

  return (0);
}

/*******************************************************************************/
/******************************** EMAX7-START **********************************/
/*******************************************************************************/

/* lmmwb=0: if lmm never be written back to DDR */
emax7_pre_with_keep_cache()
{
  /* (conf -> scon -> addr -> breg ) -> dma -> exec -> dma -> term */
#ifdef ARMSIML
  emax_pre_with_keep_cache(); /* start syscall EMAX7 */
#endif
#ifdef ARMZYNQ
  /* do nothing */
#endif
}

/* lmmwb=1: if lmm may   be written back to DDR */
emax7_pre_with_drain_cache()
{
  /* (conf -> scon -> addr -> breg ) -> dma -> exec -> dma -> term */
#ifdef ARMSIML
  emax_pre_with_drain_cache(); /* start syscall EMAX7 */
#endif
#ifdef ARMZYNQ
  /* do nothing */
#endif
}

#endif

/*******************************************************************************/
/******************************** EMAX7 NCLIB (no conv-c2c)*********************/
/*******************************************************************************/

void /*__attribute__((always_inline))*/
cex(Uint op_cx, Ull *ex, Ull c3, Ull c2, Ull c1, Ull c0, Ushort pattern)
{
  Uint index1, index0;

  switch (op_cx) {
  case OP_NOP:
    if (ex)
      *ex = 3; /* for bsim */
    break;
  case OP_CEXE:
    index1 = ((c3>>32&1)<<3)|((c2>>32&1)<<2)|((c1>>32&1)<<1)|(c0>>32&1);
    index0 = ((c3    &1)<<3)|((c2    &1)<<2)|((c1    &1)<<1)|(c0    &1);
    *ex = 0;
    if (pattern>>index1&1) *ex |= 2;
    if (pattern>>index0&1) *ex |= 1;
    break;
  default:
    printf("emax7lib: cex: undefined op_cx=%d\n", op_cx);
    break;
  }  
}

void /*__attribute__((always_inline))*/
ex4(Uint op_ex1, Ull *d, Ull *r1, Uint exp1, Ull *r2, Uint exp2, Ull *r3, Uint exp3, Uint op_ex2, Ull *r4, Uint op_ex3, Ull *r5)
{
  switch (op_ex1) {
  case OP_SFMA: /* 3in 8bit*32 stochastic r1+r2*r3 -> 8bit */
    exe(op_ex1, (d+0), (Ull)r1, exp1, *(r2+0), exp2, *(r3+0), exp3, OP_NOP, (Ull)r4, OP_NOP, (Ull)r5);
    exe(op_ex1, (d+0), *(d+0),  exp1, *(r2+1), exp2, *(r3+1), exp3, OP_NOP, (Ull)r4, OP_NOP, (Ull)r5);
    exe(op_ex1, (d+0), *(d+0),  exp1, *(r2+2), exp2, *(r3+2), exp3, OP_NOP, (Ull)r4, OP_NOP, (Ull)r5);
    exe(op_ex1, (d+0), *(d+0),  exp1, *(r2+3), exp2, *(r3+3), exp3, OP_NOP, (Ull)r4, OP_NOP, (Ull)r5);
    break;
  case OP_NOP:
  case OP_CVT53:/* 3in 8bit*2,32bit*2 for llama-q3_K */
  case OP_SML8: /* 2in 8bit*4,8bit*4 (8bit*8bit+8bit*8bit)->24bit r1*r2 */
  case OP_CFMA: /* 3in [idx|32bit]*2 (idx2==idx3)?r1+r2*r3:r1 */
  case OP_FMA:  /* 3in 32bit*2 floating-point r1+r2*r3 */
  case OP_FMS:  /* 3in 32bit*2 floating-point r1-r2*r3 */
  case OP_FML:  /* 2in 32bit*2 floating-point r1*r2 */
  case OP_FAD:  /* 2in 32bit*2 floating-point r1+r2 */
  case OP_FML3: /* 3in 32bit*2 floating-point r1*r2[idx:r3]) */
  case OP_ADD3: /* 3in 32bit*2 fixed-point r1+(r2+r3) */
  case OP_SUB3: /* 3in 32bit*2 fixed-point r1-(r2+r3) */
  case OP_ADD:  /* 2in 32bit*2 fixed-point r1+r2 */
  case OP_SUB:  /* 2in 32bit*2 fixed-point r1-r2 */
    exe(op_ex1, (d+0), *(r1+0), exp1, *(r2+0), exp2, *(r3+0), exp3, OP_NOP, 0LL, OP_NOP, 0LL);
    exe(op_ex1, (d+1), *(r1+1), exp1, *(r2+1), exp2, *(r3+1), exp3, OP_NOP, 0LL, OP_NOP, 0LL);
    exe(op_ex1, (d+2), *(r1+2), exp1, *(r2+2), exp2, *(r3+2), exp3, OP_NOP, 0LL, OP_NOP, 0LL);
    exe(op_ex1, (d+3), *(r1+3), exp1, *(r2+3), exp2, *(r3+3), exp3, OP_NOP, 0LL, OP_NOP, 0LL);
    break;
  default:
    printf("emax7lib: ex4: undefined op_ex1=%d\n", op_ex1);
    break;
  }

  switch (op_ex2) {
  case OP_NOP:
    break;
  default:
    printf("emax7lib: ex4: illegal op_ex2=%d\n", op_ex2);
    break;
  }

  switch (op_ex3) {
  case OP_NOP:
    break;
  default:
    printf("emax7lib: ex4: illegal op_ex3=%d\n", op_ex3);
    break;
  }
}

int convf32tou7(Uchar *out, float in)
{
  //  convf32tou7 e=126     0.992 -> s0111111  0111111111111111111111111111111111111111111111111111111111111111
  //  convf32tou7 e=126 f=0 0.500 -> s0100000  0000000000000000000000000000000011111111111111111111111111111111
  //  convf32tou7 e=125 f=0 0.250 -> s0010000  0000000000000000000000000000000000000000000000001111111111111111
  //  convf32tou7 e=124 f=0 0.125 -> s0001000  0000000000000000000000000000000000000000000000000000000011111111
  //  convf32tou7 e=123 f=0 0.062 -> s0000100  0000000000000000000000000000000000000000000000000000000000001111
  //  convf32tou7 e=122 f=0 0.031 -> s0000010  0000000000000000000000000000000000000000000000000000000000000011
  //  convf32tou7 e=121 f=0 0.016 -> s0000001  0000000000000000000000000000000000000000000000000000000000000001
  //                        0.000 -> s0000000  0000000000000000000000000000000000000000000000000000000000000000
  f32bit in_f32;
  wu7bit out_u7;

  *(float*)&in_f32 = in;

  out_u7.s = in_f32.s;
  out_u7.b = 0;

  in = abs(in);
  if  (in >= 1.0) out_u7.e = 63;    /* �軻����6bitɽ��(-1.0+1.0) */
  else            out_u7.e = in*64; /* number of 1 */    

  *out = *(Uchar*)&out_u7;
//printf("%7.4f -> %02.2x\n", *(float*)&in_f32, *out);
}

int convf32tou8(Uchar *out, float in)
{
  f32bit in_f32;
  wu8bit out_u8;

  *(float*)&in_f32 = in;

  out_u8.s = in_f32.s;

  in = abs(in);
  if  (in >= 2.0) out_u8.e = 127;   /* �軻����6bitɽ��(-1.0+1.0) */
  else            out_u8.e = in*64; /* number of 1 */    

  *out = *(Uchar*)&out_u8;
//printf("%7.4f -> %02.2x\n", *(float*)&in_f32, *out);
}

int convu8tof32(float *out, Uchar in)
{
  wu8bit in_u8;
  f32bit out_f32;

  *(Uchar*)&in_u8 = in;
  *(float*)&out_f32 = (float)in_u8.e/64; /* 6bitɽ��(-2.0+2.0) */
  out_f32.s = in_u8.s;
  *out = *(float*)&out_f32;

//printf("%02.2x -> %7.4f\n", in, *out);
}

Ull urand(int no)
{
  static Ull urand_seed[8]
    = {0xc3c3c3c3a5a5a5a5LL, 0x123456789abcdef0LL, 0xe1e1e1e1d4d4d4d4LL, 0x8888777766665555LL,
       0x8787878796969696LL, 0xfedcba9876543210LL, 0x5a5a5a5a3c3c3c3cLL, 0xbbbbccccddddeeeeLL};
  Ull retval = urand_seed[no];

//urand_seed = urand_seed * 1103515245LL + 12345LL;

  urand_seed[no] ^= (urand_seed[no]<<29);
  urand_seed[no] ^= (urand_seed[no]>>27);
  urand_seed[no] ^= (urand_seed[no]<<37);
  return (retval);
}

Ull shfl(Ull in, Ull r)
{
  int i;
  for (i=0; i<32; i++) {
    if (r&(1LL<<(i+16)))
      in = (in&~(1LL<<(i+32)|1LL<<i)) | (in>>i&1)<<(i+32) | (in>>(i+32)&1)<<i;
  }
  for (i=0; i<48; i++) {
    if (r&(1LL<<(i+8)))
      in = (in&~(1LL<<(i+16)|1LL<<i)) | (in>>i&1)<<(i+16) | (in>>(i+16)&1)<<i;
  }
  for (i=0; i<56; i++) {
    if (r&(1LL<<(i+4)))
      in = (in&~(1LL<<(i+ 8)|1LL<<i)) | (in>>i&1)<<(i+ 8) | (in>>(i+ 8)&1)<<i;
  }
  for (i=0; i<60; i++) {
    if (r&(1LL<<(i+2)))
      in = (in&~(1LL<<(i+ 4)|1LL<<i)) | (in>>i&1)<<(i+ 4) | (in>>(i+ 4)&1)<<i;
  }
  for (i=0; i<62; i++) {
    if (r&(1LL<<(i+1)))
      in = (in&~(1LL<<(i+ 2)|1LL<<i)) | (in>>i&1)<<(i+ 2) | (in>>(i+ 2)&1)<<i;
  }
  for (i=0; i<63; i++) {
    if (r&(1LL<<(i+0)))
      in = (in&~(1LL<<(i+ 1)|1LL<<i)) | (in>>i&1)<<(i+ 1) | (in>>(i+ 1)&1)<<i;
  }
  return(in);
}

int enable_x11;    /* 0 or set in extern */

void x11_softu64_dist(float, float);
int softu64(int stage, Ull *o1, Ull *o2, Ull *o3, Ull r1, Ull r2, Ull r3, Ull r4) /* o <- s1 + s2 * s3 */
     /* stage:1 stage_2 in EXEC:  r2*r3 64bit*2  -> *o1 32bit*8 b mult     */
     /* stage:2 stage_3 in EXEC:  *o1,r4 32bit*8 -> *o2 8bit+8bit count up */
     /* stage:3 stage_4 in EXEC:  r1 + *o2��     -> *o3 8bit               */
{
  int i, j;
  Ull u[8];
  Ull ss[8];
  Ull s2[8], s3[8];
  int pc, nc; /* number of 1 */
  int os, oc;

//#define SPU_DATA_BITS 31
//#define SPU_DATA_DIST 2
//#define SPU_COUT_BITS 31
#define SPU_DATA_BITS 15
#define SPU_DATA_DIST 4
#define SPU_COUT_BITS 12

  switch (stage) {
  case 1: /* stage2 */
    for (i=0; i<8; i++) /* s2 * s3 -> ad2 */
      u[i] = urand(i);
    for (i=0; i<8; i++) { /* s2 * s3 -> ad2 */
      ss[i] = (r2>>(i*8+7))&1 ^ (r3>>(i*8+7))&1;
  int s2e   = (r2>>(i*8))&0x7f; s2e = s2e<SPU_DATA_BITS?s2e:SPU_DATA_BITS;
  int s3e   = (r3>>(i*8))&0x7f; s3e = s3e<SPU_DATA_BITS?s3e:SPU_DATA_BITS;
#if 0
      s2[i] = (Ull)0x7fffffffffffffffLL>>(63-s2e); //�軻��6bit*6bit->6bit
      s3[i] = (Ull)0x7fffffffffffffffLL>>(63-s3e); //�軻��6bit*6bit->6bit
      // ����64bit��ǥ���åե�
      s2[i] = shfl(s2[i], u[2]);
      s3[i] = shfl(s3[i], u[3]);
#else
      // �����SPU_DATA_WIDTH bit��˻���.�����ͤ�6bit���ۤȤ��15�ʲ��Ǥ��뤳�Ȥ�����(63�᤯�ʤ�ȸ������Ф�Ϥ�)
      s2[i] = 0LL;
      s3[i] = 0LL;
      for (j=0; j<SPU_COUT_BITS; j++) {
	int k = j * SPU_DATA_DIST; /* SPU_DATA_BITS=15�ʤ�4bit�� */
	s2[i] |= ((u[(i+0)%8]>>k&SPU_DATA_BITS)<=s2e)<<j;
	s3[i] |= ((u[(i+1)%8]>>k&SPU_DATA_BITS)<=s3e)<<j;
      }
      //printf("%08.8x_%08.8x %08.8x_%08.8x %d:%08.8x %d:%08.8x\n", (Uint)(u2>>32), (Uint)u2, (Uint)(u3>>32), (Uint)u3, s2e, (Uint)s2[i], s3e, (Uint)s3[i]);
#endif
      // s2*s3 �����Ǥ�stochastic�軻
      o1[i] = s2[i] & s3[i];                         // 1*1=1�ˤʤ� �ºݤϾ��SPU_DATA_BITS�Τ�AND
      o1[i] = ss[i]<<63|(o1[i]&0x7fffffffffffffffLL);// stage2�ν��Ϥ�(��Ƭ���bit|SPU_DATA_BITS bit) * 8
    }
    break;
  case 2: /* stage3 */
    pc = 0;
    nc = 0;
    // ����/������롼�פ��Ȥˡ�������ʬ�򥹥ʥåץ���å�
    for (j=0; j<SPU_COUT_BITS; j++) {
      for (i=0; i<8; i++) { /* s2 * s3 -> ad2 */
	if (!(o1[i]>>63)) pc += (o1[i] & (1LL<<j))!=0;
	else              nc += (o1[i] & (1LL<<j))!=0;
      }
    }
    pc = pc>>r4; // r4=3 for MNIST/CIFAR10
    nc = nc>>r4; // r4=2 for test021
    *o2 = (Ull)(pc&0xffff)<<32 | (Ull)(nc&0xffff);
    break;
  case 3: /* stage4 */
    pc = *o2>>32&0xffff; /* high */
    nc = *o2    &0xffff; /* low */
    // s1�򤵤�˲û�
    if (!(r1&0x80)) pc += (r1&0x7f); /* merge pos s1 s1.e�Ϻ���7bit */
    else            nc += (r1&0x7f); /* merge neg s1 s1.e�Ϻ���7bit */
    // ����������βû�(s1:7bit + s2*s3:6bit->7bit)
    if (pc >= nc) {
      os = 0x00; /* pos */
      oc = pc-nc; /* # of 1 */
    }
    else {
      os = 0x80; /* neg */
      oc = nc-pc; /* # of 1 */
    }
    if (oc >= 128) oc = 127;
    *o3 = os|oc;
#if !defined(ARMSIML) && defined(TRACE_SPIKE)
    if (enable_x11) {
      int i;
      Uchar r2_u8;
      Uchar r3_u8;
      float r1_f32;
      float r2_f32;
      float r3_f32;
      float o3_f32;
      convu8tof32(&o3_f32, *(Uchar*)o3);   /* for graph */
      convu8tof32(&r1_f32, *(Uchar*)&r1); /* for graph */
      for (i=0; i<8; i++) { /* s2 * s3 -> ad2 */
	r2_u8 = r2>>(i*8)&0xff;
	r3_u8 = r3>>(i*8)&0xff;
	convu8tof32(&r2_f32, r2_u8); /* for graph */
	convu8tof32(&r3_f32, r3_u8); /* for graph */
	r1_f32 += r2_f32*r3_f32;
      }
      x11_softu64_dist(r1_f32, o3_f32);
    }
#endif
    break;
  }

  return (0);
}

Ull /*__attribute__((always_inline))*/
exm(Ull s, Uchar exp)
{
  switch (exp) {
  case EXP_H3210: return ( s );
  case EXP_H1010: return ((s<<32&0xffffffff00000000LL) | (s    &0x00000000ffffffffLL));
  case EXP_H3232: return ((s    &0xffffffff00000000LL) | (s>>32&0x00000000ffffffffLL));
  case EXP_B7632: return ((s>> 8&0x00ff000000ff0000LL) | (s>>16&0x000000ff000000ffLL));
  case EXP_B5410: return ((s<< 8&0x00ff000000ff0000LL) | (s    &0x000000ff000000ffLL));
  default:        return ( s );
  }
}

int /*__attribute__((always_inline))*/
exe(Uint op_ex1, Ull *d, Ull s1, Uint exp1, Ull s2, Uint exp2, Ull s3, Uint exp3, Uint op_ex2, Ull r4, Uint op_ex3, Ull r5)
{
  /* return 0:normal, 1:OP_WHILE breaks */
  union { Uint i; float f; } f3, f2, f1, f0;
  Ull r1, r2, r3;
  Ull t3, t2, t1, t0;
  short h3, h2, h1, h0;
  int w3, w2, w1, w0;
  Ull ro00, ro01, ro02, ro10, ro11, ro12;
  Ull c1, c0;
  Ull ex1_outd;
  Ull ex1_outd_sfma[8];
  Ull ex2_outd;
  int retval = 0;
  float convi4f32[16] = {-8.0, -7.0, -6.0, -5.0, -4.0, -3.0, -2.0, -1.0, 0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0};

  if (op_ex1 == OP_CVT53) {
    r1 = s1>>(exp1&~1)&0x0000030000000300LL | s1>>(exp1&~3)&0x0000000f0000000fLL;
    r2 = s2>> exp1;      /* 0-7 */
    r3 = s3>>(exp1&3)*2; /* 0,2,4,6 */
  }
  else { /* normal */
    r1 = exm(s1, exp1);
    r2 = exm(s2, exp2);
    r3 = exm(s3, exp3);
  }

  switch (op_ex1) {
  case OP_NOP:
    ex1_outd = r1;
    break;
  case OP_WHILE: /* emax7nc��lib�Ȥ��Ƥϻ��Ѥ���,bsim/emax7.c��siml�˻��� */
    t0 = (r1&0x00000000ffffffffLL)+(r2&0x00000000ffffffffLL);
    t0 &= 0x00000000ffffffffLL;
    ex1_outd = t0;
    if (t0==0) retval = 1;
    break;
  case OP_FOR: /* emax7nc��lib�Ȥ��Ƥϻ��Ѥ���,bsim/emax7.c��siml�˻��� */
    t0 = (r1&0x00000000ffffffffLL)+(r2&0x00000000ffffffffLL);
    t0 &= 0x00000000ffffffffLL;
    ex1_outd = t0;
    if (t0==0) retval = 1;
    break;
  case OP_CVT53:/* 3in 8bit*2,32bit*2 for llama-q3_K */
    /* shift���r1,2,3������EXP_CVT00 src1(sc)=-----s-c|-----s-c  src2(hm)=..|-------h -------h -------h -------h  src3(qs)=..|------qq ------qq ------qq ------qq */
    t2 = ((Ull)(((signed char)((r1>>36&0x30|r1>>32&0x0f)>>1) - 16) * ((signed char)(r3>>56&0x03)-(r2>>56&0x01?0:4)))&0xff)<<24
       | ((Ull)(((signed char)((r1>>36&0x30|r1>>32&0x0f)>>1) - 16) * ((signed char)(r3>>48&0x03)-(r2>>48&0x01?0:4)))&0xff)<<16
       | ((Ull)(((signed char)((r1>>36&0x30|r1>>32&0x0f)>>1) - 16) * ((signed char)(r3>>40&0x03)-(r2>>40&0x01?0:4)))&0xff)<< 8
       | ((Ull)(((signed char)((r1>>36&0x30|r1>>32&0x0f)>>1) - 16) * ((signed char)(r3>>32&0x03)-(r2>>32&0x01?0:4)))&0xff)<< 0;
    t0 = ((Ull)(((signed char)((r1>> 4&0x30|r1>> 0&0x0f)>>1) - 16) * ((signed char)(r3>>24&0x03)-(r2>>24&0x01?0:4)))&0xff)<<24
       | ((Ull)(((signed char)((r1>> 4&0x30|r1>> 0&0x0f)>>1) - 16) * ((signed char)(r3>>16&0x03)-(r2>>16&0x01?0:4)))&0xff)<<16
       | ((Ull)(((signed char)((r1>> 4&0x30|r1>> 0&0x0f)>>1) - 16) * ((signed char)(r3>> 8&0x03)-(r2>> 8&0x01?0:4)))&0xff)<< 8
       | ((Ull)(((signed char)((r1>> 4&0x30|r1>> 0&0x0f)>>1) - 16) * ((signed char)(r3>> 0&0x03)-(r2>> 0&0x01?0:4)))&0xff)<< 0;
    ex1_outd = (t2<<32)|(t0);
    break;
  case OP_SML8: /* 2in 8bit*4,8bit*4 (8bit*8bit+8bit*8bit)->24bit r1*r2 */
    h3 = (signed short)((signed char)(r1>>48&0x00000000000000ffLL)*(signed char)(r2>>48&0x00000000000000ffLL));
    h2 = (signed short)((signed char)(r1>>32&0x00000000000000ffLL)*(signed char)(r2>>32&0x00000000000000ffLL));
    h1 = (signed short)((signed char)(r1>>16&0x00000000000000ffLL)*(signed char)(r2>>16&0x00000000000000ffLL));
    h0 = (signed short)((signed char)(r1    &0x00000000000000ffLL)*(signed char)(r2    &0x00000000000000ffLL));
    w2 = (int)h3+(int)h2;
    w0 = (int)h1+(int)h0;
    t2 = w2 & 0x00ffffff; /* 24bit */
    t0 = w0 & 0x00ffffff; /* 24bit */
    ex1_outd = (t2<<32)|(t0);
    break;
  case OP_SFMA: /* 3in 8bit*32 stochastic r1+r2*r3 -> 8bit */
    softu64(1, ex1_outd_sfma, NULL, NULL, r1, r2, r3, r4);
    break;
  case OP_CFMA: /* 3in [idx|32bit]*2 (idx2==idx3)?r1+r2*r3:r1 */
    f1.i = (Uint)(r1);
    f2.i = (Uint)(r2>>32);
    f3.i = (Uint)(r3>>32);
    if (f2.i != -1 && f2.i == f3.i) {
      f2.i = (Uint)(r2);
      f3.i = (Uint)(r3);
      f0.f = f1.f + (f2.f * f3.f);
    }
    else {
      f0.f = f1.f;
    }
    t0 = f0.i;
    ex1_outd = t0;
    break;
  case OP_FMA: /* 3in 32bit*2 floating-point r1+r2*r3 */
  case OP_FMS: /* 3in 32bit*2 floating-point r1-r2*r3 */
    /* *(double*)&ex1_outd = *(double*)&r1 + (*(double*)&r2 * *(double*)&r3);*/
    f1.i = (Uint)(r1>>32);
    f2.i = (Uint)(r2>>32)^(op_ex1==OP_FMA?0:0x80000000);
    f3.i = (Uint)(r3>>32);
    f0.f = f1.f + (f2.f * f3.f);
    t2 = f0.i;
    f1.i = (Uint)(r1);
    f2.i = (Uint)(r2)^(op_ex1==OP_FMA?0:0x80000000);
    f3.i = (Uint)(r3);
    f0.f = f1.f + (f2.f * f3.f);
    t0 = f0.i;
    ex1_outd = (t2<<32)|(t0);
    break;
  case OP_FML: /* 2in 32bit*2 floating-point r1*r2 */
    /* *(double*)&ex1_outd = *(double*)&r1 * *(double*)&r2;*/
    f1.i = (Uint)(r1>>32);
    f2.i = (Uint)(r2>>32);
    f0.f = f1.f * f2.f;
    t2 = f0.i;
    f1.i = (Uint)(r1);
    f2.i = (Uint)(r2);
    f0.f = f1.f * f2.f;
    t0 = f0.i;
    ex1_outd = (t2<<32)|(t0);
    break;
  case OP_FAD: /* 2in 32bit*2 floating-point r1+r2 */
    /* *(double*)&ex1_outd = *(double*)&r1 + *(double*)&r2;*/
    f1.i = (Uint)(r1>>32);
    f2.i = (Uint)(r2>>32);
    f0.f = f1.f + f2.f;
    t2 = f0.i;
    f1.i = (Uint)(r1);
    f2.i = (Uint)(r2);
    f0.f = f1.f + f2.f;
    t0 = f0.i;
    ex1_outd = (t2<<32)|(t0);
    break;
  case OP_FML3: /* 3in 32bit*2 floating-point r1*r2[idx:r3] */
    /* *(double*)&ex1_outd = *(double*)&r1 * *(int*)&r2[idx];*/
    f1.i = (Uint)(r1>>32);
    f2.i = (Uint)(r2>>32);
    f3.i = (Uint)(r3>>32);
    f0.f = f1.f * convi4f32[(f2.i>>((f3.i&7)*4))&0xf];
    t2 = f0.i;
    f1.i = (Uint)(r1);
    f2.i = (Uint)(r2);
    f3.i = (Uint)(r3);
    f0.f = f1.f * convi4f32[(f2.i>>((f3.i&7)*4))&0xf];
    t0 = f0.i;
    ex1_outd = (t2<<32)|(t0);
    break;
  case OP_ADD3: /* 3in 32bit*2 integer add s1+(s2+s3) */
    t2 = (r1>>32&0x00000000ffffffffLL)+((r2>>32&0x00000000ffffffffLL)+(r3>>32&0x00000000ffffffffLL));
    t2 &= 0x00000000ffffffffLL;
    t0 = (r1    &0x00000000ffffffffLL)+((r2    &0x00000000ffffffffLL)+(r3    &0x00000000ffffffffLL));
    t0 &= 0x00000000ffffffffLL;
    ex1_outd = (t2<<32)|(t0);
    break;
  case OP_SUB3: /* 3in 32bit*2 integer subtract s1-(s2+s3) */
    t2 = (r1>>32&0x00000000ffffffffLL)-((r2>>32&0x00000000ffffffffLL)+(r3>>32&0x00000000ffffffffLL));
    t2 &= 0x00000000ffffffffLL;
    t0 = (r1    &0x00000000ffffffffLL)-((r2    &0x00000000ffffffffLL)+(r3    &0x00000000ffffffffLL));
    t0 &= 0x00000000ffffffffLL;
    ex1_outd = (t2<<32)|(t0);
    break;
  case OP_ADD: /* 2in 32bit*2 integer add s1+s2 */
    t2 = (r1>>32&0x00000000ffffffffLL)+(r2>>32&0x00000000ffffffffLL);
    t2 &= 0x00000000ffffffffLL;
    t0 = (r1    &0x00000000ffffffffLL)+(r2    &0x00000000ffffffffLL);
    t0 &= 0x00000000ffffffffLL;
    ex1_outd = (t2<<32)|(t0);
    break;
  case OP_SUB: /* 2in 32bit*2 integer subtract s1-s2 */
    t2 = (r1>>32&0x00000000ffffffffLL)-(r2>>32&0x00000000ffffffffLL);
    t2 &= 0x00000000ffffffffLL;
    t0 = (r1    &0x00000000ffffffffLL)-(r2    &0x00000000ffffffffLL);
    t0 &= 0x00000000ffffffffLL;
    ex1_outd = (t2<<32)|(t0);
    break;
  case OP_CMP_EQ: /* 2in 32bit*2 compare and set 1*2bit-CC */
    c1 = (r1>>32&0x00000000ffffffffLL) == (r2>>32&0x00000000ffffffffLL);
    c0 = (r1    &0x00000000ffffffffLL) == (r2    &0x00000000ffffffffLL);
    ex1_outd = (c1<<32)|c0;
    break;
  case OP_CMP_NE: /* 2in 32bit*2 compare and set 1*2bit-CC */
    c1 = (r1>>32&0x00000000ffffffffLL) != (r2>>32&0x00000000ffffffffLL);
    c0 = (r1    &0x00000000ffffffffLL) != (r2    &0x00000000ffffffffLL);
    ex1_outd = (c1<<32)|c0;
    break;
  case OP_CMP_LT: /* 2in 32bit*2 compare and set 1*2bit-CC */
    c1 = (r1>>32&0x00000000ffffffffLL) < (r2>>32&0x00000000ffffffffLL);
    c0 = (r1    &0x00000000ffffffffLL) < (r2    &0x00000000ffffffffLL);
    ex1_outd = (c1<<32)|c0;
    break;
  case OP_CMP_LE: /* 2in 32bit*2 compare and set 1*2bit-CC */
    c1 = (r1>>32&0x00000000ffffffffLL) <= (r2>>32&0x00000000ffffffffLL);
    c0 = (r1    &0x00000000ffffffffLL) <= (r2    &0x00000000ffffffffLL);
    ex1_outd = (c1<<32)|c0;
    break;
  case OP_CMP_GT: /* 2in 32bit*2 compare and set 1*2bit-CC */
    c1 = (r1>>32&0x00000000ffffffffLL) > (r2>>32&0x00000000ffffffffLL);
    c0 = (r1    &0x00000000ffffffffLL) > (r2    &0x00000000ffffffffLL);
    ex1_outd = (c1<<32)|c0;
    break;
  case OP_CMP_GE: /* 2in 32bit*2 compare and set 1*2bit-CC */
    c1 = (r1>>32&0x00000000ffffffffLL) >= (r2>>32&0x00000000ffffffffLL);
    c0 = (r1    &0x00000000ffffffffLL) >= (r2    &0x00000000ffffffffLL);
    ex1_outd = (c1<<32)|c0;
    break;
  case OP_CMOV: /* 2in 32bit*2 word-wise(w1/w0) conditional move */
    c1 = r1>>32&1;
    c0 = r1    &1;
    t2 = c1 ? (r2&0xffffffff00000000LL) : (r3&0xffffffff00000000LL);
    t0 = c0 ? (r2&0x00000000ffffffffLL) : (r3&0x00000000ffffffffLL);
    ex1_outd = t2 | t0;
    break;
  case OP_MAUH3: /* 3in 16bit*4 r1.pos+(r2.pos+r3.pos) */
    t3 = (r1>>48&0x000000000000ffffLL)+((r2>>48&0x000000000000ffffLL)+(r3>>48&0x000000000000ffffLL));
    if (t3 > 0x000000000000ffffLL) t3 = 0x000000000000ffffLL;
    t2 = (r1>>32&0x000000000000ffffLL)+((r2>>32&0x000000000000ffffLL)+(r3>>32&0x000000000000ffffLL));
    if (t2 > 0x000000000000ffffLL) t2 = 0x000000000000ffffLL;
    t1 = (r1>>16&0x000000000000ffffLL)+((r2>>16&0x000000000000ffffLL)+(r3>>16&0x000000000000ffffLL));
    if (t1 > 0x000000000000ffffLL) t1 = 0x000000000000ffffLL;
    t0 = (r1    &0x000000000000ffffLL)+((r2    &0x000000000000ffffLL)+(r3    &0x000000000000ffffLL));
    if (t0 > 0x000000000000ffffLL) t0 = 0x000000000000ffffLL;
    ex1_outd = (t3<<48)|(t2<<32)|(t1<<16)|(t0);
    break;
  case OP_MAUH: /* 2in 16bit*4 r1.pos+r2.pos */
    t3 = (r1>>48&0x000000000000ffffLL)+(r2>>48&0x000000000000ffffLL);
    if (t3 > 0x000000000000ffffLL) t3 = 0x000000000000ffffLL;
    t2 = (r1>>32&0x000000000000ffffLL)+(r2>>32&0x000000000000ffffLL);
    if (t2 > 0x000000000000ffffLL) t2 = 0x000000000000ffffLL;
    t1 = (r1>>16&0x000000000000ffffLL)+(r2>>16&0x000000000000ffffLL);
    if (t1 > 0x000000000000ffffLL) t1 = 0x000000000000ffffLL;
    t0 = (r1    &0x000000000000ffffLL)+(r2    &0x000000000000ffffLL);
    if (t0 > 0x000000000000ffffLL) t0 = 0x000000000000ffffLL;
    ex1_outd = (t3<<48)|(t2<<32)|(t1<<16)|(t0);
    break;
  case OP_MSUH3: /* 3in 16bit*4 r1.pos-(r2.pos+r3.pos) */
    t3 = (r1>>48&0x000000000000ffffLL)-((r2>>48&0x000000000000ffffLL)+(r3>>48&0x000000000000ffffLL));
    if (t3 > 0x000000000000ffffLL) t3 = 0x0000000000000000LL;
    t2 = (r1>>32&0x000000000000ffffLL)-((r2>>32&0x000000000000ffffLL)+(r3>>32&0x000000000000ffffLL));
    if (t2 > 0x000000000000ffffLL) t2 = 0x0000000000000000LL;
    t1 = (r1>>16&0x000000000000ffffLL)-((r2>>16&0x000000000000ffffLL)+(r3>>16&0x000000000000ffffLL));
    if (t1 > 0x000000000000ffffLL) t1 = 0x0000000000000000LL;
    t0 = (r1    &0x000000000000ffffLL)-((r2    &0x000000000000ffffLL)+(r3    &0x000000000000ffffLL));
    if (t0 > 0x000000000000ffffLL) t0 = 0x0000000000000000LL;
    ex1_outd = (t3<<48)|(t2<<32)|(t1<<16)|(t0);
    break;
  case OP_MSUH: /* 2in 16bit*4 r1.pos-r2.pos */
    t3 = (r1>>48&0x000000000000ffffLL)-(r2>>48&0x000000000000ffffLL);
    if (t3 > 0x000000000000ffffLL) t3 = 0x0000000000000000LL;
    t2 = (r1>>32&0x000000000000ffffLL)-(r2>>32&0x000000000000ffffLL);
    if (t2 > 0x000000000000ffffLL) t2 = 0x0000000000000000LL;
    t1 = (r1>>16&0x000000000000ffffLL)-(r2>>16&0x000000000000ffffLL);
    if (t1 > 0x000000000000ffffLL) t1 = 0x0000000000000000LL;
    t0 = (r1    &0x000000000000ffffLL)-(r2    &0x000000000000ffffLL);
    if (t0 > 0x000000000000ffffLL) t0 = 0x0000000000000000LL;
    ex1_outd = (t3<<48)|(t2<<32)|(t1<<16)|(t0);
    break;
  case OP_MLUH: /* (11bit*4)*9bit r1.pos*r2.pos */
    t3 = (r1>>48&0x00000000000007ffLL)*(r2>>32&0x00000000000001ffLL);
    if (t3 > 0x000000000000ffffLL) t3 = 0x000000000000ffffLL;
    t2 = (r1>>32&0x00000000000007ffLL)*(r2>>32&0x00000000000001ffLL);
    if (t2 > 0x000000000000ffffLL) t2 = 0x000000000000ffffLL;
    t1 = (r1>>16&0x00000000000007ffLL)*(r2    &0x00000000000001ffLL);
    if (t1 > 0x000000000000ffffLL) t1 = 0x000000000000ffffLL;
    t0 = (r1    &0x00000000000007ffLL)*(r2    &0x00000000000001ffLL);
    if (t0 > 0x000000000000ffffLL) t0 = 0x000000000000ffffLL;
    ex1_outd = (t3<<48)|(t2<<32)|(t1<<16)|(t0);
    break;
  case OP_MMRG: /* 3in 8bit*2 r1.b4|r2.b4|r3.b4|0->w1, r1.b0|r2.b0|r3.b0|0->w0 */
    ex1_outd = ((r1&0x000000ff00000000LL)<<24) | ((r2&0x000000ff00000000LL)<<16) | ((r3&0x000000ff00000000LL)<<8)
             | ((r1&0x00000000000000ffLL)<<24) | ((r2&0x00000000000000ffLL)<<16) | ((r3&0x00000000000000ffLL)<<8);
    break;
  case OP_MSSAD: /* 2in 16bit*4 8bit*8 r1.h3+df(r2.b7,r3.b7)+df(r2.b6,r3.b6)->d.h3
                                       r1.h2+df(r2.b5,r3.b5)+df(r2.b4,r3.b4)->d.h2
                                       r1.h1+df(r2.b3,r3.b3)+df(r2.b2,r3.b2)->d.h1
                                       r1.h0+df(r2.b1,r3.b1)+df(r2.b0,r3.b0)->d.h0 */
    t3 = (r1>>48&0x000000000000ffffLL) + ad(r2>>56&0x00000000000000ffLL, r3>>56&0x00000000000000ffLL) + ad(r2>>48&0x00000000000000ffLL, r3>>48&0x00000000000000ffLL);
    if (t3 > 0x000000000000ffffLL) t3 = 0x000000000000ffffLL;
    t2 = (r1>>32&0x000000000000ffffLL) + ad(r2>>40&0x00000000000000ffLL, r3>>40&0x00000000000000ffLL) + ad(r2>>32&0x00000000000000ffLL, r3>>32&0x00000000000000ffLL);
    if (t2 > 0x000000000000ffffLL) t2 = 0x000000000000ffffLL;
    t1 = (r1>>16&0x000000000000ffffLL) + ad(r2>>24&0x00000000000000ffLL, r3>>24&0x00000000000000ffLL) + ad(r2>>16&0x00000000000000ffLL, r3>>16&0x00000000000000ffLL);
    if (t1 > 0x000000000000ffffLL) t1 = 0x000000000000ffffLL;
    t0 = (r1    &0x000000000000ffffLL) + ad(r2>> 8&0x00000000000000ffLL, r3>> 8&0x00000000000000ffLL) + ad(r2    &0x00000000000000ffLL, r3    &0x00000000000000ffLL);
    if (t0 > 0x000000000000ffffLL) t0 = 0x000000000000ffffLL;
    ex1_outd = (t3<<48)|(t2<<32)|(t1<<16)|(t0);
    break;
  case OP_MSAD: /* 2in 16bit*4 8bit*8 df(r1.b7,r2.b7)+df(r1.b6,r2.b6)->d.h3
                                      df(r1.b5,r2.b5)+df(r1.b4,r2.b4)->d.h2
                                      df(r1.b3,r2.b3)+df(r1.b2,r2.b2)->d.h1
                                      df(r1.b1,r2.b1)+df(r1.b0,r2.b0)->d.h0 */
    t3 = ad(r1>>56&0x00000000000000ffLL, r2>>56&0x00000000000000ffLL) + ad(r1>>48&0x00000000000000ffLL, r2>>48&0x00000000000000ffLL);
    if (t3 > 0x000000000000ffffLL) t3 = 0x000000000000ffffLL;
    t2 = ad(r1>>40&0x00000000000000ffLL, r2>>40&0x00000000000000ffLL) + ad(r1>>32&0x00000000000000ffLL, r2>>32&0x00000000000000ffLL);
    if (t2 > 0x000000000000ffffLL) t2 = 0x000000000000ffffLL;
    t1 = ad(r1>>24&0x00000000000000ffLL, r2>>24&0x00000000000000ffLL) + ad(r1>>16&0x00000000000000ffLL, r2>>16&0x00000000000000ffLL);
    if (t1 > 0x000000000000ffffLL) t1 = 0x000000000000ffffLL;
    t0 = ad(r1>> 8&0x00000000000000ffLL, r2>> 8&0x00000000000000ffLL) + ad(r1    &0x00000000000000ffLL, r2    &0x00000000000000ffLL);
    if (t0 > 0x000000000000ffffLL) t0 = 0x000000000000ffffLL;
    ex1_outd = (t3<<48)|(t2<<32)|(t1<<16)|(t0);
    break;
  case OP_MINL3: /* 3in 16bit*4 (r3.h3<r3.h2)?r1.h3|r3.h3:r2.h3|r3.h2->d.w1
                                (r3.h1<r3.h0)?r1.h1|r3.h1:r2.h1|r3.h0->d.w0 */
    t3 = r3>>48&0x000000000000ffffLL;
    t2 = r3>>32&0x000000000000ffffLL;
    t1 = r3>>16&0x000000000000ffffLL;
    t0 = r3    &0x000000000000ffffLL;
    if (t3<t2) t2 = (r1&0xffff000000000000LL)|(r3>>16&0x0000ffff00000000LL);
    else       t2 = (r2&0xffff000000000000LL)|(r3    &0x0000ffff00000000LL);
    if (t1<t0) t0 = (r1&0x00000000ffff0000LL)|(r3>>16&0x000000000000ffffLL);
    else       t0 = (r2&0x00000000ffff0000LL)|(r3    &0x000000000000ffffLL);
    ex1_outd = t2 | t0;
    break;
  case OP_MINL: /* 2in 16bit*4 (r1.h2<r2.h2)?r1.w1:r2.w1->d.w1
	                       (r1.h0<r2.h0)?r1.w0:r2.w0->d.w0 */
    if ((r1&0x0000ffff00000000LL)<(r2&0x0000ffff00000000LL)) t2 = r1&0xffffffff00000000LL;
    else                                                     t2 = r2&0xffffffff00000000LL;
    if ((r1&0x000000000000ffffLL)<(r2&0x000000000000ffffLL)) t0 = r1&0x00000000ffffffffLL;
    else                                                     t0 = r2&0x00000000ffffffffLL;
    ex1_outd = t2 | t0;
   break;
  case OP_MH2BW: /* 2in 16bit*4 r1.b6|r1.b4|r2.b6|r2.b4|r1.b2|r1.b0|r2.b2|r2.b0 */
    ex1_outd = (((r1>>48&0x000000000000ff00LL) ? 255 : (r1>>48&0x00000000000000ffLL))<<56)
             | (((r1>>32&0x000000000000ff00LL) ? 255 : (r1>>32&0x00000000000000ffLL))<<48)
             | (((r2>>48&0x000000000000ff00LL) ? 255 : (r2>>48&0x00000000000000ffLL))<<40)
             | (((r2>>32&0x000000000000ff00LL) ? 255 : (r2>>32&0x00000000000000ffLL))<<32)
             | (((r1>>16&0x000000000000ff00LL) ? 255 : (r1>>16&0x00000000000000ffLL))<<24)
             | (((r1    &0x000000000000ff00LL) ? 255 : (r1    &0x00000000000000ffLL))<<16)
             | (((r2>>16&0x000000000000ff00LL) ? 255 : (r2>>16&0x00000000000000ffLL))<< 8)
             | (((r2    &0x000000000000ff00LL) ? 255 : (r2    &0x00000000000000ffLL))    );
    break;
  case OP_MCAS: /* 2in 16bit*2 (r1.h2<r2.h2)?0:0xff->d.b1
                               (r1.h0<r2.h0)?0:0xff->d.b0 */
    t2 = ((r1&0x0000ffff00000000LL)<(r2&0x0000ffff00000000LL))?0:0x000000ff00000000LL;
    t0 = ((r1&0x000000000000ffffLL)<(r2&0x000000000000ffffLL))?0:0x00000000000000ffLL;
    ex1_outd = t2 | t0;
    break;
  case OP_MMID3: /* 3in 8bit*8 bytewise compare and output middle */
    t1 = ((r1&0xff00000000000000LL)<(r2&0xff00000000000000LL)?(r1&0xff00000000000000LL):(r2&0xff00000000000000LL))
       | ((r1&0x00ff000000000000LL)<(r2&0x00ff000000000000LL)?(r1&0x00ff000000000000LL):(r2&0x00ff000000000000LL))
       | ((r1&0x0000ff0000000000LL)<(r2&0x0000ff0000000000LL)?(r1&0x0000ff0000000000LL):(r2&0x0000ff0000000000LL))
       | ((r1&0x000000ff00000000LL)<(r2&0x000000ff00000000LL)?(r1&0x000000ff00000000LL):(r2&0x000000ff00000000LL))
       | ((r1&0x00000000ff000000LL)<(r2&0x00000000ff000000LL)?(r1&0x00000000ff000000LL):(r2&0x00000000ff000000LL))
       | ((r1&0x0000000000ff0000LL)<(r2&0x0000000000ff0000LL)?(r1&0x0000000000ff0000LL):(r2&0x0000000000ff0000LL))
       | ((r1&0x000000000000ff00LL)<(r2&0x000000000000ff00LL)?(r1&0x000000000000ff00LL):(r2&0x000000000000ff00LL))
       | ((r1&0x00000000000000ffLL)<(r2&0x00000000000000ffLL)?(r1&0x00000000000000ffLL):(r2&0x00000000000000ffLL));
    t2 = ((r1&0xff00000000000000LL)>(r2&0xff00000000000000LL)?(r1&0xff00000000000000LL):(r2&0xff00000000000000LL))
       | ((r1&0x00ff000000000000LL)>(r2&0x00ff000000000000LL)?(r1&0x00ff000000000000LL):(r2&0x00ff000000000000LL))
       | ((r1&0x0000ff0000000000LL)>(r2&0x0000ff0000000000LL)?(r1&0x0000ff0000000000LL):(r2&0x0000ff0000000000LL))
       | ((r1&0x000000ff00000000LL)>(r2&0x000000ff00000000LL)?(r1&0x000000ff00000000LL):(r2&0x000000ff00000000LL))
       | ((r1&0x00000000ff000000LL)>(r2&0x00000000ff000000LL)?(r1&0x00000000ff000000LL):(r2&0x00000000ff000000LL))
       | ((r1&0x0000000000ff0000LL)>(r2&0x0000000000ff0000LL)?(r1&0x0000000000ff0000LL):(r2&0x0000000000ff0000LL))
       | ((r1&0x000000000000ff00LL)>(r2&0x000000000000ff00LL)?(r1&0x000000000000ff00LL):(r2&0x000000000000ff00LL))
       | ((r1&0x00000000000000ffLL)>(r2&0x00000000000000ffLL)?(r1&0x00000000000000ffLL):(r2&0x00000000000000ffLL));
    ex1_outd = ((r3&0xff00000000000000LL)<(t1&0xff00000000000000LL)?(t1&0xff00000000000000LL):((r3&0xff00000000000000LL)<(t2&0xff00000000000000LL)?(r3&0xff00000000000000LL):(t2&0xff00000000000000LL)))
             | ((r3&0x00ff000000000000LL)<(t1&0x00ff000000000000LL)?(t1&0x00ff000000000000LL):((r3&0x00ff000000000000LL)<(t2&0x00ff000000000000LL)?(r3&0x00ff000000000000LL):(t2&0x00ff000000000000LL)))
             | ((r3&0x0000ff0000000000LL)<(t1&0x0000ff0000000000LL)?(t1&0x0000ff0000000000LL):((r3&0x0000ff0000000000LL)<(t2&0x0000ff0000000000LL)?(r3&0x0000ff0000000000LL):(t2&0x0000ff0000000000LL)))
             | ((r3&0x000000ff00000000LL)<(t1&0x000000ff00000000LL)?(t1&0x000000ff00000000LL):((r3&0x000000ff00000000LL)<(t2&0x000000ff00000000LL)?(r3&0x000000ff00000000LL):(t2&0x000000ff00000000LL)))
             | ((r3&0x00000000ff000000LL)<(t1&0x00000000ff000000LL)?(t1&0x00000000ff000000LL):((r3&0x00000000ff000000LL)<(t2&0x00000000ff000000LL)?(r3&0x00000000ff000000LL):(t2&0x00000000ff000000LL)))
             | ((r3&0x0000000000ff0000LL)<(t1&0x0000000000ff0000LL)?(t1&0x0000000000ff0000LL):((r3&0x0000000000ff0000LL)<(t2&0x0000000000ff0000LL)?(r3&0x0000000000ff0000LL):(t2&0x0000000000ff0000LL)))
             | ((r3&0x000000000000ff00LL)<(t1&0x000000000000ff00LL)?(t1&0x000000000000ff00LL):((r3&0x000000000000ff00LL)<(t2&0x000000000000ff00LL)?(r3&0x000000000000ff00LL):(t2&0x000000000000ff00LL)))
             | ((r3&0x00000000000000ffLL)<(t1&0x00000000000000ffLL)?(t1&0x00000000000000ffLL):((r3&0x00000000000000ffLL)<(t2&0x00000000000000ffLL)?(r3&0x00000000000000ffLL):(t2&0x00000000000000ffLL)));
    break;
  case OP_MMAX3: /* 3in 8bit*8 bytewise compare and output maximum */
    t1 = ((r1&0xff00000000000000LL)>(r2&0xff00000000000000LL)?(r1&0xff00000000000000LL):(r2&0xff00000000000000LL))
       | ((r1&0x00ff000000000000LL)>(r2&0x00ff000000000000LL)?(r1&0x00ff000000000000LL):(r2&0x00ff000000000000LL))
       | ((r1&0x0000ff0000000000LL)>(r2&0x0000ff0000000000LL)?(r1&0x0000ff0000000000LL):(r2&0x0000ff0000000000LL))
       | ((r1&0x000000ff00000000LL)>(r2&0x000000ff00000000LL)?(r1&0x000000ff00000000LL):(r2&0x000000ff00000000LL))
       | ((r1&0x00000000ff000000LL)>(r2&0x00000000ff000000LL)?(r1&0x00000000ff000000LL):(r2&0x00000000ff000000LL))
       | ((r1&0x0000000000ff0000LL)>(r2&0x0000000000ff0000LL)?(r1&0x0000000000ff0000LL):(r2&0x0000000000ff0000LL))
       | ((r1&0x000000000000ff00LL)>(r2&0x000000000000ff00LL)?(r1&0x000000000000ff00LL):(r2&0x000000000000ff00LL))
       | ((r1&0x00000000000000ffLL)>(r2&0x00000000000000ffLL)?(r1&0x00000000000000ffLL):(r2&0x00000000000000ffLL));
    ex1_outd = ((t1&0xff00000000000000LL)>(r3&0xff00000000000000LL)?(t1&0xff00000000000000LL):(r3&0xff00000000000000LL))
             | ((t1&0x00ff000000000000LL)>(r3&0x00ff000000000000LL)?(t1&0x00ff000000000000LL):(r3&0x00ff000000000000LL))
             | ((t1&0x0000ff0000000000LL)>(r3&0x0000ff0000000000LL)?(t1&0x0000ff0000000000LL):(r3&0x0000ff0000000000LL))
             | ((t1&0x000000ff00000000LL)>(r3&0x000000ff00000000LL)?(t1&0x000000ff00000000LL):(r3&0x000000ff00000000LL))
             | ((t1&0x00000000ff000000LL)>(r3&0x00000000ff000000LL)?(t1&0x00000000ff000000LL):(r3&0x00000000ff000000LL))
             | ((t1&0x0000000000ff0000LL)>(r3&0x0000000000ff0000LL)?(t1&0x0000000000ff0000LL):(r3&0x0000000000ff0000LL))
             | ((t1&0x000000000000ff00LL)>(r3&0x000000000000ff00LL)?(t1&0x000000000000ff00LL):(r3&0x000000000000ff00LL))
             | ((t1&0x00000000000000ffLL)>(r3&0x00000000000000ffLL)?(t1&0x00000000000000ffLL):(r3&0x00000000000000ffLL));
    break;
  case OP_MMIN3: /* 3in 8bit*8 bytewise compare and output minimum */
    t1 = ((r1&0xff00000000000000LL)<(r2&0xff00000000000000LL)?(r1&0xff00000000000000LL):(r2&0xff00000000000000LL))
       | ((r1&0x00ff000000000000LL)<(r2&0x00ff000000000000LL)?(r1&0x00ff000000000000LL):(r2&0x00ff000000000000LL))
       | ((r1&0x0000ff0000000000LL)<(r2&0x0000ff0000000000LL)?(r1&0x0000ff0000000000LL):(r2&0x0000ff0000000000LL))
       | ((r1&0x000000ff00000000LL)<(r2&0x000000ff00000000LL)?(r1&0x000000ff00000000LL):(r2&0x000000ff00000000LL))
       | ((r1&0x00000000ff000000LL)<(r2&0x00000000ff000000LL)?(r1&0x00000000ff000000LL):(r2&0x00000000ff000000LL))
       | ((r1&0x0000000000ff0000LL)<(r2&0x0000000000ff0000LL)?(r1&0x0000000000ff0000LL):(r2&0x0000000000ff0000LL))
       | ((r1&0x000000000000ff00LL)<(r2&0x000000000000ff00LL)?(r1&0x000000000000ff00LL):(r2&0x000000000000ff00LL))
       | ((r1&0x00000000000000ffLL)<(r2&0x00000000000000ffLL)?(r1&0x00000000000000ffLL):(r2&0x00000000000000ffLL));
    ex1_outd = ((t1&0xff00000000000000LL)<(r3&0xff00000000000000LL)?(t1&0xff00000000000000LL):(r3&0xff00000000000000LL))
             | ((t1&0x00ff000000000000LL)<(r3&0x00ff000000000000LL)?(t1&0x00ff000000000000LL):(r3&0x00ff000000000000LL))
             | ((t1&0x0000ff0000000000LL)<(r3&0x0000ff0000000000LL)?(t1&0x0000ff0000000000LL):(r3&0x0000ff0000000000LL))
             | ((t1&0x000000ff00000000LL)<(r3&0x000000ff00000000LL)?(t1&0x000000ff00000000LL):(r3&0x000000ff00000000LL))
             | ((t1&0x00000000ff000000LL)<(r3&0x00000000ff000000LL)?(t1&0x00000000ff000000LL):(r3&0x00000000ff000000LL))
             | ((t1&0x0000000000ff0000LL)<(r3&0x0000000000ff0000LL)?(t1&0x0000000000ff0000LL):(r3&0x0000000000ff0000LL))
             | ((t1&0x000000000000ff00LL)<(r3&0x000000000000ff00LL)?(t1&0x000000000000ff00LL):(r3&0x000000000000ff00LL))
             | ((t1&0x00000000000000ffLL)<(r3&0x00000000000000ffLL)?(t1&0x00000000000000ffLL):(r3&0x00000000000000ffLL));
    break;
  case OP_MMAX: /* 2in 8bit*8 bytewise compare and output maximum */
    ex1_outd = ((r1&0xff00000000000000LL)>(r2&0xff00000000000000LL)?(r1&0xff00000000000000LL):(r2&0xff00000000000000LL))
             | ((r1&0x00ff000000000000LL)>(r2&0x00ff000000000000LL)?(r1&0x00ff000000000000LL):(r2&0x00ff000000000000LL))
             | ((r1&0x0000ff0000000000LL)>(r2&0x0000ff0000000000LL)?(r1&0x0000ff0000000000LL):(r2&0x0000ff0000000000LL))
             | ((r1&0x000000ff00000000LL)>(r2&0x000000ff00000000LL)?(r1&0x000000ff00000000LL):(r2&0x000000ff00000000LL))
             | ((r1&0x00000000ff000000LL)>(r2&0x00000000ff000000LL)?(r1&0x00000000ff000000LL):(r2&0x00000000ff000000LL))
             | ((r1&0x0000000000ff0000LL)>(r2&0x0000000000ff0000LL)?(r1&0x0000000000ff0000LL):(r2&0x0000000000ff0000LL))
             | ((r1&0x000000000000ff00LL)>(r2&0x000000000000ff00LL)?(r1&0x000000000000ff00LL):(r2&0x000000000000ff00LL))
             | ((r1&0x00000000000000ffLL)>(r2&0x00000000000000ffLL)?(r1&0x00000000000000ffLL):(r2&0x00000000000000ffLL));
    break;
  case OP_MMIN: /* 2in 8bit*8 bytewise compare and output minimum */
    ex1_outd = ((r1&0xff00000000000000LL)<(r2&0xff00000000000000LL)?(r1&0xff00000000000000LL):(r2&0xff00000000000000LL))
             | ((r1&0x00ff000000000000LL)<(r2&0x00ff000000000000LL)?(r1&0x00ff000000000000LL):(r2&0x00ff000000000000LL))
             | ((r1&0x0000ff0000000000LL)<(r2&0x0000ff0000000000LL)?(r1&0x0000ff0000000000LL):(r2&0x0000ff0000000000LL))
             | ((r1&0x000000ff00000000LL)<(r2&0x000000ff00000000LL)?(r1&0x000000ff00000000LL):(r2&0x000000ff00000000LL))
             | ((r1&0x00000000ff000000LL)<(r2&0x00000000ff000000LL)?(r1&0x00000000ff000000LL):(r2&0x00000000ff000000LL))
             | ((r1&0x0000000000ff0000LL)<(r2&0x0000000000ff0000LL)?(r1&0x0000000000ff0000LL):(r2&0x0000000000ff0000LL))
             | ((r1&0x000000000000ff00LL)<(r2&0x000000000000ff00LL)?(r1&0x000000000000ff00LL):(r2&0x000000000000ff00LL))
             | ((r1&0x00000000000000ffLL)<(r2&0x00000000000000ffLL)?(r1&0x00000000000000ffLL):(r2&0x00000000000000ffLL));
    break;
  case OP_MAJ: /* (((x) & (y))^((x) & (z))^((y) & (z))) */
    ex1_outd = (r1&0xffffffff00000000LL) | (((r1 & r2)^(r1 & r3)^(r2 & r3))&0xffffffffLL);
    break;
  case OP_CH: /*  (((x) & (y))^(~(x) & (z))) */
    ex1_outd = (r1&0xffffffff00000000LL) | (((r1 & r2)^(~r1 & r3))&0xffffffffLL);
    break;
  default:
    printf("emax7lib: exe: undefined op_ex1=%d\n", op_ex1);
    break;
  }

  switch (op_ex2) {
  case OP_NOP:
    if (op_ex1 == OP_SFMA)
      softu64(2, ex1_outd_sfma, &ex2_outd, NULL, r1, r2, r3, r4);
    else
      ex2_outd = ex1_outd;
    break;
  case OP_AND: /* 2in 64bit logical and s1&s2 */
    ex2_outd = ex1_outd & r4;
    break;
  case OP_OR: /* 2in 64bit logical or s1|s2 */
    ex2_outd = ex1_outd | r4;
    break;
  case OP_XOR: /* 2in 64bit logical xor s1^s2 */
    ex2_outd = ex1_outd ^ r4;
    break;
  case OP_SUMHH: /* 1in 16bit*4 & s1.h3+s1.h2->d.h3, s1.h1+s1.h0->d.h1 */
    t3 = ex1_outd>>48&0x000000000000ffffLL;
    t2 = ex1_outd>>32&0x000000000000ffffLL;
    t1 = ex1_outd>>16&0x000000000000ffffLL;
    t0 = ex1_outd    &0x000000000000ffffLL;
    t3 += t2;
    if (t3 > 0x000000000000ffffLL) t3 = 0x000000000000ffffLL;
    t1 += t0;
    if (t1 > 0x000000000000ffffLL) t1 = 0x000000000000ffffLL;
    ex2_outd = (t3<<48)|(t1<<16);
    break;
  case OP_SUMHL: /* 1in 16bit*4 & s1.h3+s1.h2->d.h2, s1.h1+s1.h0->d.h0 */
    t3 = ex1_outd>>48&0x000000000000ffffLL;
    t2 = ex1_outd>>32&0x000000000000ffffLL;
    t1 = ex1_outd>>16&0x000000000000ffffLL;
    t0 = ex1_outd    &0x000000000000ffffLL;
    t2 += t3;
    if (t2 > 0x000000000000ffffLL) t2 = 0x000000000000ffffLL;
    t0 += t1;
    if (t0 > 0x000000000000ffffLL) t0 = 0x000000000000ffffLL;
    ex2_outd = (t2<<32)|(t0);
    break;
  case OP_AD24: /* 2in 24bit*2,24bit*2 24bit+24bit->24bit r1+r2 |OP_SRAB */
    w2 = ((int)(ex1_outd>>24)>>8)+((int)(r4>>24)>>8);
    w0 = ((int)(ex1_outd<< 8)>>8)+((int)(r4<< 8)>>8);
    t2 = w2 & 0x00ffffff; /* 24bit */
    t0 = w0 & 0x00ffffff; /* 24bit */
    ex2_outd = (t2<<32)|(t0);
    break;
//case OP_WSWAP: /* 32bit 2in swap and mask words */
//  ex2_outd = ((ex1_outd<<32)|(ex1_outd>>32)) & r4;
//  break;
  case OP_ROTS: /* hi-32bit #define ROTRIGHT (((a) >> (b)) | ((a) << (32-(b)))) */
    t2 = ex1_outd & 0xffffffff00000000LL;
    ro10 = r4>>32 & 0xff;
    ro11 = r4>>40 & 0xff;
    ro12 = r4>>48 & 0xff;
    t0 = ex1_outd & 0x00000000ffffffffLL;
    ro00 = r4     & 0xff;
    ro01 = r4>> 8 & 0xff;
    ro02 = r4>>16 & 0xff;
    ex2_outd = (((t2>>ro12|t2<<(32-ro12))^(t2>>ro11|t2<<(32-ro11))^(t2>>ro10|t2<<(32-ro10)))&0xffffffff00000000LL)
              |(((t0>>ro02|t0<<(32-ro02))^(t0>>ro01|t0<<(32-ro01))^(t0>>ro00|t0<<(32-ro00)))&0x00000000ffffffffLL);
    break;
  default:
    printf("emax7lib: exe: undefined op_ex2=%d\n", op_ex2);
    break;
  }

  switch (op_ex3) {
  case OP_NOP:
    if (op_ex1 == OP_SFMA)
      softu64(3, NULL, &ex2_outd, d, r1, r2, r3, r4);
    else
      if (d) *d = ex2_outd;
    break;
  case OP_SLL: /* 2in 32bit*2 32bit logical shift to left */
    t1 = (Ull)(ex2_outd    &0xffffffff00000000LL)<<r5;
    t0 = (Ull)(ex2_outd<<r5&0x00000000ffffffffLL);
    if (d) *d = t1 | t0;
    break;
  case OP_SRL: /* 2in 32bit*2 32bit logical shift to right */
    t1 = (Ull)(ex2_outd>>r5&0xffffffff00000000LL);
    t0 = (Ull)(ex2_outd    &0x00000000ffffffffLL)>>r5;
    if (d) *d = t1 | t0;
    break;
  case OP_SRAA: /* 2in 32bit*2 32bit arith shift to right (bit63,31 is ext.) */
    t1 = (Sll)(ex2_outd    )>>r5&0xffffffff00000000LL;
    t0 = (Sll)(ex2_outd<<32)>>r5&0xffffffff00000000LL;
    if (d) *d = t1 | (t0>>32);
    break;
  case OP_SRAB: /* 2in 32bit*2 32bit arith shift to right (bit55,23 is ext.) */
    t1 = (Sll)(ex2_outd<< 8)>>(r5+8)&0xffffffff00000000LL;
    t0 = (Sll)(ex2_outd<<40)>>(r5+8)&0xffffffff00000000LL;
    if (d) *d = t1 | (t0>>32);
    break;
//case OP_SRAC: /* 2in 32bit*2 32bit arith shift to right (bit47,15 is ext.) */
//  t1 = (Sll)(ex2_outd<<16)>>(r5+16)&0xffffffff00000000LL;
//  t0 = (Sll)(ex2_outd<<48)>>(r5+16)&0xffffffff00000000LL;
//  if (d) *d = t1 | (t0>>32);
//  break;
//case OP_SRAD: /* 2in 32bit*2 32bit arith shift to right (bit39,7 is ext.) */
//  t1 = (Sll)(ex2_outd<<24)>>(r5+24)&0xffffffff00000000LL;
//  t0 = (Sll)(ex2_outd<<56)>>(r5+24)&0xffffffff00000000LL;
//  if (d) *d = t1 | (t0>>32);
//  break;
  case OP_SRLM: /* 2in 16bit*4 16bit arith shift to right */
    t3 = (Ull)(ex2_outd    )>>r5&0xffff000000000000LL;
    t2 = (Ull)(ex2_outd<<16)>>r5&0xffff000000000000LL;
    t1 = (Ull)(ex2_outd<<32)>>r5&0xffff000000000000LL;
    t0 = (Ull)(ex2_outd<<48)>>r5&0xffff000000000000LL;
    if (d) *d = t3 | (t2>>16) | (t1>>32) | (t0>>48);
    break;
  default:
    printf("emax7lib: exe: undefined op_ex3=%d\n", op_ex3);
    break;
  }

  return (retval);
}

void /*__attribute__((always_inline))*/
mex(Uint op_mex2, Uchar **d2, Uchar *base2, Ull ofs2, Uint op_mex1, Uchar **d1, Uchar *base1, Ull ofs1, Ull limit, Ull s2, Ull s1)
{
  /* limit:  0, 8, 16, .... 4096, 8192, 16384, 32768     */
  /* encode: 0, 1, 2,  3,   10    11    12     13 (4bit) */
  Uint limit2 = limit*2;
  Uint ss2 = s2>>32;
  Uint ss1 = s1>>32;

  switch (op_mex1) {
  case OP_NOP:
    *d1 = base1;
    break;
  case OP_ALWAYS: /* base++ �б� */
    *d1 = base1 + ofs1;
    break;
  case OP_CMPA_GE:
    //d1��ʬ(ss1)��ffffffff�ʤ����. base1==limit2�ʤ����. base2==limit �ʤ�����
    if (!limit) /* sparse matrix */
      *d1 = base1 + ((ss1!=0xffffffff && ss2>=ss1) ? ofs1:0);
    else { /* merge sort */
      if ((base2==limit && base1+ofs1==limit2)||(base2+ofs2==limit && base1==limit2))
	*d1 = limit;
      else
	*d1 = base1 + (base1!=limit2 && ((base2!=limit  && ss2>=ss1)||base2==limit ) ? ofs1:0);
    }
    break;
  default:
    printf("emax7lib: mex: undefined op_mex1=%d\n", op_mex1);
    break;
  }  

  switch (op_mex2) {
  case OP_NOP:
    *d2 = base2;
    break;
  case OP_ALWAYS: /* base++ �б� */
    *d2 = base2 + ofs2;
    break;
  case OP_CMPA_LE:
    //d2��ʬ(ss2)��ffffffff�ʤ����. base2==limit �ʤ����. base1==limit2�ʤ�����
    if (!limit) /* sparse matrix */
      *d2 = base2 + ((ss2!=0xffffffff && ss2<=ss1) ? ofs2:0);
    else { /* merge sort */
      if ((base2==limit && base1+ofs1==limit2)||(base2+ofs2==limit && base1==limit2))
	*d2 = 0;
      else
	*d2 = base2 + (base2!=limit  && ((base1!=limit2 && ss2<=ss1)||base1==limit2) ? ofs2:0);
    }
    break;
  default:
    printf("emax7lib: mex: undefined op_mex2=%d\n", op_mex2);
    break;
  }  
}

Ull /*__attribute__((always_inline))*/
eam(Ull ofs, Uchar msk)
{
  switch (msk) {
  case  MSK_D0: return (ofs);
  case  MSK_W1: return (ofs>>32);
  case  MSK_W0: return (ofs&0x00000000ffffffffLL);
  case  MSK_H3: return (ofs>>48&0x000000000000ffffLL);
  case  MSK_H2: return (ofs>>32&0x000000000000ffffLL);
  case  MSK_H1: return (ofs>>16&0x000000000000ffffLL);
  case  MSK_H0: return (ofs&0x000000000000ffffLL);
  case  MSK_B7: return (ofs>>56&0x00000000000000ffLL);
  case  MSK_B6: return (ofs>>48&0x00000000000000ffLL);
  case  MSK_B5: return (ofs>>40&0x00000000000000ffLL);
  case  MSK_B4: return (ofs>>32&0x00000000000000ffLL);
  case  MSK_B3: return (ofs>>24&0x00000000000000ffLL);
  case  MSK_B2: return (ofs>>16&0x00000000000000ffLL);
  case  MSK_B1: return (ofs>>8&0x00000000000000ffLL);
  case  MSK_B0: return (ofs&0x00000000000000ffLL);
  default: printf("emax7lib: eag: undefined msk=%d\n", msk); return (0LL);;
  }
}

void /*__attribute__((always_inline))*/
eag(Ull *adr, Ull base, Ull ofs)
{
  *adr = base + ofs;
}

void /*__attribute__((always_inline))*/
mop(Uint op_mm, Ull ex, Ull *d, Ull base, Ull offset, Uchar msk, Ull top, Uint len, Uint blk, Uchar force, Ull ptop, Uint plen)
{
  Ull adr, ofs;

  eag(&adr, base, eam(offset, msk));
  mmp(op_mm, ex, d, adr, top, len, blk);
}

void /*__attribute__((always_inline))*/
mo4(Uint op_mm, Ull ex, Ull *d, Ull base, Ull offset, Uchar msk, Ull top, Uint len, Uint blk, Uchar force, Ull ptop, Uint plen)
{
  Ull adr, ofs;

  eag(&adr, base, eam(offset, msk));
  mmp(op_mm, ex, d, adr, top, len, blk);
}

int emax7_unaligned_load_valid; /* mop(BR[][][1]adr+8); mop(BR[][][0]adr);Ϣ³��������ꤷ,1�ξ��high��ͭ��,0�ξ��̵��  */
Ull emax7_unaligned_load_high;  /* mop(BR[][][1]adr+8); mop(BR[][][0]adr);Ϣ³��������ꤷ,high¦��켡��¸��low¦��merge */

void /*__attribute__((always_inline))*/
mmp(Uint op_mm, Ull ex, Ull *d, Ull adr, Ull top, Uint len, Uint blk)
{
  Ull c1, c0, load64;

#if defined(__i386)
  adr &= (1LL<<32)-1;
  top &= (1LL<<32)-1;
#endif  

  if (!((op_mm==OP_LDRQ && blk) || op_mm==OP_LDDMQ || op_mm==OP_TR) && (!adr || !top)) return; /* NULL skip DMA */

#define CHECK_MMP_MARGIN 12
  if (!((op_mm==OP_LDRQ && blk) || op_mm==OP_LDDMQ || op_mm==OP_TR) && ex && len && (adr < top || adr >= top+len*sizeof(Uint)+CHECK_MMP_MARGIN)) {
    printf("mmp: adr=%08.8x_%08.8x out of range (top=%08.8x_%08.8x len=%dB)\n", (Uint)(adr>>32), (Uint)adr, (Uint)(top>>32), (Uint)top, len*sizeof(Uint));
    fflush(stdout);
  }

  c1 = ex>>1&1;
  c0 = ex   &1;

  switch (op_mm) {
  case OP_NOP:
    break;

    /* MOP */
  case OP_LDR: /* 64bit lmm LMM is preloaded, random-access */
    load64 = *(Ull*)(adr&~7LL);
    if ((adr&7) == 0)
      *d = load64;
    else if (!emax7_unaligned_load_valid) { /* BR[][][1] */
      emax7_unaligned_load_valid = 1;
      emax7_unaligned_load_high = load64;
      *d = load64 >> (adr&7)*8;
    }
    else { /* BR[][][0] */
      emax7_unaligned_load_valid = 0; 
      *d = emax7_unaligned_load_high << (8-(adr&7))*8 | load64 >> (adr&7)*8;
    }
    break;
  case OP_LDWR: /* u32bit lmm LMM is preloaded, random-access */
    *d = (Ull)*(Uint*)(adr&~3LL);
    break;
//case OP_LDHR: /* u16bit lmm LMM is preloaded, random-access */
//  *d = (Ull)(Uint)*(Ushort*)(adr&~1LL);
//  break;
  case OP_LDBR: /* u8bit lmm LMM is preloaded, random-access */
    *d = (Ull)(Uint)*(Uchar*)adr;
    break;
  case OP_STR: /* 64bit lmm LMM is drained. random-access */
    if (c1) *((Uint*)(adr&~7LL)+1) = *d>>32;
    if (c0) *((Uint*)(adr&~7LL)  ) = *d;
    break;
  case OP_STWR: /* 32bit lmm LMM is drained. random-access */
    if (c0) *(Uint*)(adr&~3LL) = *d;
    break;
//case OP_STHR: /* 16bit lmm LMM is drained. random-access */
//  if (c0) *(Ushort*)(adr&~1LL) = *d;
//  break;
  case OP_STBR: /* 8bit lmm LMM is drained. random-access */
    if (c0) *(Uchar*)adr = *d;
    break;

    /* MO4 */
  case OP_LDRQ: /* 64bit*4 lmm LMM is preloaded, random-access */
    switch (blk) {
    case 0: /* normal */
      /* adr=0,32,64,... */
      *(d+0) = *((Ull*)(adr&~31LL)+0);
      *(d+1) = *((Ull*)(adr&~31LL)+1);
      *(d+2) = *((Ull*)(adr&~31LL)+2);
      *(d+3) = *((Ull*)(adr&~31LL)+3);
      break;
    case 1: /* block_size=16-members */
      /* adr=0,32,64,... memadr = mem(top + (adr/32/16*ptr)) + (adr/32/&15)*4 */
      *(d+0) = *(*(Ull**)(top + (adr/32/16*sizeof(Ull*))) + (adr/32&15)*4 + 0);
      *(d+1) = *(*(Ull**)(top + (adr/32/16*sizeof(Ull*))) + (adr/32&15)*4 + 1);
      *(d+2) = *(*(Ull**)(top + (adr/32/16*sizeof(Ull*))) + (adr/32&15)*4 + 2);
      *(d+3) = *(*(Ull**)(top + (adr/32/16*sizeof(Ull*))) + (adr/32&15)*4 + 3);
      break;
    case 2: /* block_size=32-members */
      /* adr=0,32,64,... memadr = mem(top + (adr/32/32*ptr)) + (adr/32/&31)*4 */
      *(d+0) = *(*(Ull**)(top + (adr/32/32*sizeof(Ull*))) + (adr/32&31)*4 + 0);
      *(d+1) = *(*(Ull**)(top + (adr/32/32*sizeof(Ull*))) + (adr/32&31)*4 + 1);
      *(d+2) = *(*(Ull**)(top + (adr/32/32*sizeof(Ull*))) + (adr/32&31)*4 + 2);
      *(d+3) = *(*(Ull**)(top + (adr/32/32*sizeof(Ull*))) + (adr/32&31)*4 + 3);
      break;
    default:/* block_size=64-members */
      /* adr=0,32,64,... memadr = mem(top + (adr/32/64*ptr)) + (adr/32/&63)*4 */
      *(d+0) = *(*(Ull**)(top + (adr/32/64*sizeof(Ull*))) + (adr/32&63)*4 + 0);
      *(d+1) = *(*(Ull**)(top + (adr/32/64*sizeof(Ull*))) + (adr/32&63)*4 + 1);
      *(d+2) = *(*(Ull**)(top + (adr/32/64*sizeof(Ull*))) + (adr/32&63)*4 + 2);
      *(d+3) = *(*(Ull**)(top + (adr/32/64*sizeof(Ull*))) + (adr/32&63)*4 + 3);
      break;
    }
    break;
  case OP_LDDMQ: /* 64bit*4 mem Direct access to MM */
    if (c0) {
      *(d+0) = *((Ull*)(adr&~31LL)+0);
      *(d+1) = *((Ull*)(adr&~31LL)+1);
      *(d+2) = *((Ull*)(adr&~31LL)+2);
      *(d+3) = *((Ull*)(adr&~31LL)+3);
    }
    break;
  case OP_STRQ: /* 64bit*4 lmm LMM is drained. random-access */
    *((Ull*)(adr&~31LL)+0) = *(d+0);
    *((Ull*)(adr&~31LL)+1) = *(d+1);
    *((Ull*)(adr&~31LL)+2) = *(d+2);
    *((Ull*)(adr&~31LL)+3) = *(d+3);
    break;
  case OP_TR: /* 64bit*4 exec Send transaction */
    /* addr��transaction()����˻��� */
    if (c0) {
      Ull (*trans)() = top;
      ((void (*)(Ull, Ull, Ull, Ull))trans)(*(d+0), *(d+1), *(d+2), *(d+3));
    }
    break;
  default:
    printf("emax7lib: mmp: undefined op_mm=%d\n", op_mm);
    break;
  }
}
#endif
