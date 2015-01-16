/*
 * Driver for NAND support, Rick Bronson
 * borrowed heavily from:
 * (c) 1999 Machine Vision Holdings, Inc.
 * (c) 1999, 2000 David Woodhouse <dwmw2@infradead.org>
 *
 * Ported 'dynenv' to 'nand env.oob' command
 * (C) 2010 Nanometrics, Inc.
 * 'dynenv' -- Dynamic environment offset in NAND OOB
 * (C) Copyright 2006-2007 OpenMoko, Inc.
 * Added 16-bit nand support
 * (C) 2004 Texas Instruments
 *
 * Copyright 2010 Freescale Semiconductor
 * The portions of this file whose copyright is held by Freescale and which
 * are not considered a derived work of GPL v2-only code may be distributed
 * and/or modified under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 */

 /*
 A. 加密解密
 1. 密钥的产生
     1) 找出两个相异的大素数P和Q，令N＝P×Q，M＝（P－1）*（Q－1）。
     2) 找出与M互素的大数E，且E<M；用欧氏算法计算出大数D，使D×E≡1 MOD M。
     3) 丢弃P和Q，公开E，D和N。E和N即加密密钥(公钥)，D和N即解密密钥(私钥)。
 2. 加密的步骤
     1) 计算N的有效位数tn（以字节数计），将最高位的零忽略掉，令tn1＝tn－1。比如N＝0x012A05，其有效位数tn＝5，tn1＝4
     2) 将明文数据A分割成tn1位（以字节数计）的块，每块看成一个大数，块数记为bn。从而，保证了每块都小于N。
     3) 对A的每一块Ai进行Bi＝Ai^E MOD N运算。Bi就是密文数据的一块，将所有密文块合并起来，就得到了密文数据B。
 3. 解密的步骤
     1) 同加密的第一步。
     2) 将密文数据B分割成tn位（以字节数计）的块，每块看成一个大数，块数记为bn。
     3) 对B的每一块Bi进行Ci＝Bi^D MOD N运算。Ci就是密文数据的一块，将所有密文块合并起来，就得到了密文数据C。
 */

#include <common.h>
#include <command.h>
#include <malloc.h>
#include <superblock.h>
#include <fastboot.h>
#include <sys_partition.h>

typedef struct public_key_pairs_t
{
 unsigned int  public_key;     // e
 unsigned int  divider;      // n
}
public_key_pairs;

typedef struct private_key_pairs_t
{
 unsigned int  private_key;    // d
 unsigned int  divider;      // n
}
private_key_pairs;

#define  P   (127)
#define  Q   (401)
#define  N   ((P) * (Q))
#define  M   ((P-1) * (Q-1))
#define  E   (53)

public_key_pairs   pblc_keys;
private_key_pairs  prvt_keys;

void rsa_dump(void);

static unsigned int  probe_gcd(unsigned int  divdend, unsigned int  divder)
{
     unsigned int  ret = divdend % divder;

     while(ret)
     {
         divdend = divder;
         divder  = ret;
         ret = divdend % divder;
     }

     return divder;
}

unsigned probe_high_level_power_mod(unsigned int  base_value, unsigned int  power, unsigned int  divider)
{
     unsigned int ret = 1;

     base_value %= divider;
     while(power > 0)
     {
         if(power & 1)
         {
             ret = (ret * base_value) % divider;
         }
         power /= 2;
         base_value = (base_value * base_value) % divider;
     }

     return ret;
}

unsigned rsa_init(void)
{
     unsigned int k;
     unsigned int product;
     unsigned int m_value;

     m_value = M;

     k = 1;
     if(probe_gcd(m_value, E) == 1)      //e,M互质
     {
         do
         {
             product = M * k + 1;
             if(!(product % E))
             {
                 pblc_keys.public_key = E;
                 pblc_keys.divider = N;

                 prvt_keys.private_key = product/E;
                 prvt_keys.divider = N;

#ifdef DEBUG_MODE
                 rsa_dump();
#endif

                 return 0;
             }
             k ++;
         }
         while(1);
     }

     return -1;
}


void rsa_dump(void)
{
     printf("base value\n");
     printf("M = %d(%d * %d), N = %d(%d * %d)\n", M, P-1, Q-1, N, P, Q);

     printf("public key: \n");
     printf("{e, n} = %d, %d\n", pblc_keys.public_key, pblc_keys.divider);

     printf("private key: \n");
     printf("{d, n} = %d, %d\n", prvt_keys.private_key, prvt_keys.divider);
}

void rsa_encrypt( unsigned int *input, unsigned int length, unsigned int *output )
{
     unsigned int i;

     for(i=0;i<length;i++)
     {
         output[i] = probe_high_level_power_mod(input[i], pblc_keys.public_key, pblc_keys.divider);
     }

     return ;
}


void rsa_decrypt( unsigned int *input, unsigned int length, unsigned int *output )
{
     unsigned int i;

     for(i=0;i<length;i++)
     {
         output[i] = probe_high_level_power_mod(input[i], prvt_keys.private_key, prvt_keys.divider);
     }

     return ;
}


/*
************************************************************************************************************

                                             hash function
************************************************************************************************************
*/
unsigned int cryptTable[0x500];
unsigned int seed1 = 0x7FED7FED;
unsigned int seed2 = 0xEEEEEEEE;
int key = 0;
/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :  返回数组
*
*
************************************************************************************************************
*/
void prepareCryptTable(void)
{
//	unsigned int seed = 0x00100001, index1 = 0, index2 = 0, i;
//
//	for( index1 = 0; index1 < 0x100; index1++ )
//	{
//		for( index2 = index1, i = 0; i < 5; i++, index2 += 0x100 )
//		{
//			unsigned long temp1, temp2;
//
//			seed = (seed * 125 + 3) % 0x2AAAAB;
//			temp1 = (seed & 0xFFFF) << 0x10;
//			seed = (seed * 125 + 3) % 0x2AAAAB;
//			temp2 = (seed & 0xFFFF);
//
//			cryptTable[index2] = ( temp1 | temp2 );
//		}
//	}
}

/*
************************************************************************************************************
*
*                                             function
*
*    name          :
*
*    parmeters     :
*
*    return        :
*
*    note          :  返回hash值
*
*
************************************************************************************************************
*/
unsigned int HashString( unsigned char *str, unsigned int dwHashType, unsigned int length )
{
//	unsigned char *key = (unsigned char *)str;
//	int ch;
//
//	while( length > 0 )
//	{
//		ch = *key++;
//		seed1 = cryptTable[(dwHashType << 8) + ch] ^ (seed1 + seed2);
//		seed2 = ch + seed1 + seed2 + (seed2 << 5) + 3;
//		length --;
//	}
//
//	return seed1;
	key += add_sum((void *)str, length);

	return (unsigned int)key;
}

void HashString_init(void)
{
	key = 0;
}

#define  HASH_BUFFER_BYTES                (32 * 1024)
#define  HASH_BUFFER_SECTORS              (HASH_BUFFER_BYTES/512)

static int signature_verify(const char *part_name)
{
	unsigned int tmp_start;
	unsigned int summary1, summary2;
	unsigned int s_value[4], h_value[4];
	unsigned char buffer[HASH_BUFFER_BYTES];
	unsigned int read_bytes;

	memset(buffer, 0, HASH_BUFFER_BYTES);
	//计算hash值
	prepareCryptTable();		//准备hash表
	//获取签名
	printf("ras init\n");
	rsa_init();
	printf("ras start\n");

	if(!strcmp("boot", part_name))
	{
		tmp_start = sunxi_partition_get_offset_byname(part_name);

		printf("find part %s\n", part_name);
		read_bytes = sizeof(struct fastboot_boot_img_hdr);
		if(!sunxi_flash_read(tmp_start, (read_bytes + 511)/512, buffer))
		{
			printf("signature0 read flash sig1 err\n");

			return -1;
		}
		summary1 = HashString(buffer, 1, read_bytes);	//1类hash
		read_bytes = sizeof(struct image_header);
		if(!sunxi_flash_read(tmp_start + CFG_FASTBOOT_MKBOOTIMAGE_PAGE_SIZE/512, (read_bytes + 511)/512, buffer))
		{
			printf("signature0 read flash sig2 err\n");

			return -1;
		}
		summary1 = HashString(buffer, 1, read_bytes);	//1类hash

		//获取保存的签名
		if(!sunxi_flash_read(tmp_start, 2, buffer))
		{
			printf("signature0 read flash sig3 err\n");

			return -1;
		}
		s_value[0] = *(unsigned int *)(buffer + 608);
		s_value[1] = *(unsigned int *)(buffer + 612);
		s_value[2] = *(unsigned int *)(buffer + 616);
		s_value[3] = *(unsigned int *)(buffer + 620);

		rsa_decrypt( s_value, 4, h_value );

		summary2 = (h_value[0]<<0) | (h_value[1]<<8) | (h_value[2]<<16) | (h_value[3]<<24);
#if 0
		for(j=0;j<4;j++)
		{
			printf("s_value[%d] = %x\n", j, s_value[j]);
		}
		for(j=0;j<4;j++)
		{
			printf("h_value[%d] = %x\n", j, h_value[j]);
		}
#endif
		printf("summary by hash %x\n", summary1);
		printf("summary by rsa %x\n", summary2);
		if(summary1 != summary2)
		{
			printf("boot signature invalid\n");

			return -1;
		}
	}
	else if(!strcmp("system", part_name))
	{
		struct ext4_super_block  *sblock;

		tmp_start = sunxi_partition_get_offset_byname(part_name);
		printf("find part %s\n", part_name);

		printf("find system part\n");
		HashString_init();

		read_bytes = sizeof(struct ext4_super_block);
		if(!sunxi_flash_read(tmp_start + CFG_SUPER_BLOCK_SECTOR, (read_bytes + 511)/512, buffer))
		{
			printf("signature1 read flash sig1 err\n");

			return -1;
		}
		sblock = (struct ext4_super_block *)buffer;
#if 0
		{
			int k;
			printf("s_inodes_count        = %x\n", sblock->s_inodes_count);
			printf("s_blocks_count_lo     = %x\n", sblock->s_blocks_count_lo);
			printf("s_r_blocks_count_lo   = %x\n", sblock->s_r_blocks_count_lo);
			printf("s_free_blocks_count_lo= %x\n", sblock->s_free_blocks_count_lo);
			printf("s_free_inodes_count   = %x\n", sblock->s_free_inodes_count);
			printf("s_first_data_block    = %x\n", sblock->s_first_data_block);
			printf("s_log_block_size      = %x\n", sblock->s_log_block_size);
			printf("s_log_cluster_size    = %x\n", sblock->s_log_cluster_size);
			printf("s_blocks_per_group    = %x\n", sblock->s_blocks_per_group);
			printf("s_clusters_per_group  = %x\n", sblock->s_clusters_per_group);
			printf("s_inodes_per_group    = %x\n", sblock->s_inodes_per_group);
			printf("s_mtime               = %x\n", sblock->s_mtime);
			printf("s_wtime               = %x\n", sblock->s_wtime);
			printf("s_mnt_count           = %x\n", sblock->s_mnt_count);
			printf("s_max_mnt_count       = %x\n", sblock->s_max_mnt_count);
			printf("s_magic               = %x\n", sblock->s_magic);
			printf("s_state               = %x\n", sblock->s_state);
			printf("s_errors              = %x\n", sblock->s_errors);
			printf("s_minor_rev_level     = %x\n", sblock->s_minor_rev_level);
			printf("s_lastcheck           = %x\n", sblock->s_lastcheck);
			printf("s_checkinterval       = %x\n", sblock->s_checkinterval);
			printf("s_creator_os          = %x\n", sblock->s_creator_os);
			printf("s_rev_level           = %x\n", sblock->s_rev_level);
			printf("s_def_resuid          = %x\n", sblock->s_def_resuid);
			printf("s_def_resgid          = %x\n", sblock->s_def_resgid);

			printf("s_first_ino           = %x\n", sblock->s_first_ino);
			printf("s_inode_size          = %x\n", sblock->s_inode_size);
			printf("s_block_group_nr      = %x\n", sblock->s_block_group_nr);
			printf("s_feature_compat      = %x\n", sblock->s_feature_compat);
			printf("s_feature_incompat    = %x\n", sblock->s_feature_incompat);
			printf("s_feature_ro_compat   = %x\n", sblock->s_feature_ro_compat);
			for(k=0;k<16;k++)
			{
				printf("s_uuid[%d]        = %x\n", k, sblock->s_uuid[k]);
			}
			for(k=0;k<16;k++)
			{
				printf("s_volume_name[%d] = %x\n", k, sblock->s_volume_name[k]);
			}
			for(k=0;k<64;k++)
			{
				printf("s_last_mounted[%d]= %x\n", k, sblock->s_last_mounted[k]);
			}
			printf("s_algorithm_usage_bitmap= %x\n", sblock->s_algorithm_usage_bitmap);

			printf("s_prealloc_blocks     = %x\n", sblock->s_prealloc_blocks);
			printf("s_prealloc_dir_blocks = %x\n", sblock->s_prealloc_dir_blocks);
			printf("s_reserved_gdt_blocks = %x\n", sblock->s_reserved_gdt_blocks);

			for(k=0;k<16;k++)
			{
				printf("s_journal_uuid[%d]= %x\n", k, sblock->s_journal_uuid[k]);
			}
			printf("s_journal_inum        = %x\n", sblock->s_journal_inum);
			printf("s_journal_dev         = %x\n", sblock->s_journal_dev);
			printf("s_last_orphan         = %x\n", sblock->s_last_orphan);
			for(k=0;k<16;k++)
			{
				printf("s_hash_seed[%d]   = %x\n", k, sblock->s_hash_seed[k]);
			}
			printf("s_def_hash_version    = %x\n", sblock->s_def_hash_version);
			printf("s_jnl_backup_type     = %x\n", sblock->s_jnl_backup_type);
			printf("s_desc_size           = %x\n", sblock->s_desc_size);
			printf("s_default_mount_opts  = %x\n", sblock->s_default_mount_opts);
			printf("s_first_meta_bg       = %x\n", sblock->s_first_meta_bg);
			printf("s_mkfs_time           = %x\n", sblock->s_mkfs_time);
			for(k=0;k<17;k++)
			{
				printf("s_jnl_blocks[%d]  = %x\n", k, sblock->s_jnl_blocks[k]);
			}

			printf("s_blocks_count_hi     = %x\n", sblock->s_blocks_count_hi);
			printf("s_r_blocks_count_hi   = %x\n", sblock->s_r_blocks_count_hi);
			printf("s_free_blocks_count_hi= %x\n", sblock->s_free_blocks_count_hi);
			printf("s_min_extra_isize     = %x\n", sblock->s_min_extra_isize);
			printf("s_want_extra_isize    = %x\n", sblock->s_want_extra_isize);
			printf("s_flags               = %x\n", sblock->s_flags);

			printf("s_raid_stride         = %x\n", sblock->s_raid_stride);
			printf("s_mmp_update_interval = %x\n", sblock->s_mmp_update_interval);
			printf("s_mmp_block           = %x\n", sblock->s_mmp_block);
			printf("s_raid_stripe_width   = %x\n", sblock->s_raid_stripe_width);
			printf("s_log_groups_per_flex = %x\n", sblock->s_log_groups_per_flex);
			printf("s_reserved_char_pad   = %x\n", sblock->s_reserved_char_pad);

			printf("s_reserved_pad        = %x\n", sblock->s_reserved_pad);
			printf("s_kbytes_written      = %x\n", sblock->s_kbytes_written);
			printf("s_snapshot_inum       = %x\n", sblock->s_snapshot_inum);
			printf("s_snapshot_id         = %x\n", sblock->s_snapshot_id);
			printf("s_snapshot_r_blocks_count= %x\n", sblock->s_snapshot_r_blocks_count);
			printf("s_snapshot_list       = %x\n", sblock->s_snapshot_list);
		}
#endif
		sblock->s_mtime     = CFG_SUPER_BLOCK_STAMP_VALUE;
		sblock->s_mnt_count = CFG_SUPER_BLOCK_STAMP_VALUE & 0xffff;
		memset(sblock->s_last_mounted, 0, 64);
		summary1 = HashString(buffer, 1, (unsigned int)&(((struct ext4_super_block *)0)->s_snapshot_list));	//1类hash

		//获取保存的签名
		if(!sunxi_flash_read(tmp_start, 2, buffer))
		{
			printf("signature1 read flash sig3 err\n");

			return -1;
		}
		s_value[0] = *(unsigned int *)(buffer + 1000 - 10 * 4 + 0);
		s_value[1] = *(unsigned int *)(buffer + 1000 - 10 * 4 + 4);
		s_value[2] = *(unsigned int *)(buffer + 1000 - 10 * 4 + 8);
		s_value[3] = *(unsigned int *)(buffer + 1000 - 10 * 4 + 12);

		rsa_decrypt( s_value, 4, h_value );
		summary2 = (h_value[0]<<0) | (h_value[1]<<8) | (h_value[2]<<16) | (h_value[3]<<24);
#if 0
		for(j=0;j<4;j++)
		{
			printf("s_value[%d] = %x\n", j, s_value[j]);
		}
		for(j=0;j<4;j++)
		{
			printf("h_value[%d] = %x\n", j, h_value[j]);
		}
#endif
		printf("summary by hash %x\n", summary1);
		printf("summary by rsa %x\n", summary2);
		if(summary1 != summary2)
		{
			printf("system signature invalid\n");

			return -1;
		}
	}

	return 0;
}

#define  SIG_ERASE_BUFFER_SIZE      (4 * 1024 * 1024)
#define  SIG_SMALL_BUFFER_SIZE      (32 * 1024)

int do_sunxi_boot_signature(cmd_tbl_t * cmdtp, int flag, int argc, char * const argv[])
{
	int ret;
	int m_ok = 1;

	ret = signature_verify("boot");
	if(!ret)
	{
		ret = signature_verify("system");
	}
    if(ret)
    {
    	printf("signature verify failed\n");

        __u32 private_start;
        __u32 private_size;
        __u32 erase_block_once=SIG_ERASE_BUFFER_SIZE;
        char  last_back[SIG_SMALL_BUFFER_SIZE];
        char* private_data = NULL;

		private_start = sunxi_partition_get_offset_byname("private");
		private_size  = sunxi_partition_get_size_byname("private") * 512;

		private_data = malloc(SIG_ERASE_BUFFER_SIZE);
		if(!private_data)
		{
			printf("not enough memory for sig erase\n");

			private_data = last_back;
			erase_block_once = SIG_SMALL_BUFFER_SIZE;
			m_ok = 0;
		}
        memset(private_data,0xff,erase_block_once);
        while(private_size>=erase_block_once)
        {
             sunxi_flash_write(private_start,erase_block_once/512,private_data);
             private_start += erase_block_once/512;
             private_size -= erase_block_once;
        }
        if(private_size)
        {
            sunxi_flash_write(private_start,private_size/512,private_data);
        }
        if(m_ok)
        {
        	free(private_data);
    	}
    }

    return 0;

}

U_BOOT_CMD(
	sunxi_boot_signature, CONFIG_SYS_MAXARGS, 1, do_sunxi_boot_signature,
	"sunxi_boot_signature sub-system",
	"no parmeters : \n"

);
