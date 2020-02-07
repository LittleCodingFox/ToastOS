/*
 * aarch64-rpi/fs.h
 *
 * Copyright (C) 2017 - 2020 bzt (bztsrc@gitlab)
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * This file is part of the BOOTBOOT Protocol package.
 * @brief Filesystem drivers for initial ramdisk.
 *
 */

#ifdef _FS_Z_H_
/**
 * FS/Z initrd (OS/Z's native file system)
 */
file_t fsz_initrd(unsigned char *initrd_p, char *kernel)
{
    FSZ_SuperBlock *sb = (FSZ_SuperBlock *)initrd_p;
    file_t ret = { NULL, 0 };
    if(initrd_p==NULL || memcmp(sb->magic,FSZ_MAGIC,4) || kernel==NULL){
        return ret;
    }
    unsigned char passphrase[256],chk[32],iv[32];
    int i,j,k,l,ss=1<<(sb->logsec+11);
    FSZ_DirEnt *ent;
    FSZ_Inode *in=(FSZ_Inode *)(initrd_p+sb->rootdirfid*ss);
    SHA256_CTX ctx;
    DBG(" * FS/Z ");
    DBG(kernel);
    DBG("\n");
    //decrypt initrd
    if(sb->enchash!=0 && FSZ_SB_EALG(sb->flags)!=0) {
        puts("BOOTBOOT-PANIC: Unsupported cipher\n");
        return ret;
    }
    while(sb->enchash!=0) {
        puts(" * Passphrase? ");
        l=ReadLine(passphrase,sizeof(passphrase));
        if(!l) {
            puts("\n");
            return ret;
        }
        if(sb->enchash!=crc32_calc((char*)passphrase,l)) {
            puts("\rBOOTBOOT-ERROR: Bad passphrase\n");
            continue;
        }
        puts("\r * Decrypting...\r");
        SHA256_Init(&ctx);
        SHA256_Update(&ctx,passphrase,l);
        SHA256_Update(&ctx,&sb->magic,6);
        SHA256_Final(chk,&ctx);
        for(i=0;i<sizeof(sb->encrypt);i++) sb->encrypt[i]^=chk[i];
        SHA256_Init(&ctx);
        SHA256_Update(&ctx,&sb->encrypt,sizeof(sb->encrypt));
        SHA256_Final(iv,&ctx);
        for(k=ss,j=1;j<sb->numsec;j++) {
            memcpy(chk,iv,32);
            for(i=0;i<ss;i++) {
                if(i%32==0) {
                    SHA256_Init(&ctx);
                    SHA256_Update(&ctx,&chk,32);
                    SHA256_Update(&ctx,&j,4);
                    SHA256_Final(chk,&ctx);
                }
                initrd_p[k++]^=chk[i%32]^iv[i%32];
            }
        }
        memset(sb->encrypt,0,sizeof(sb->encrypt)+4);
        sb->checksum=crc32_calc((char *)sb->magic,508);
        puts("                \r");
    }
    // Get the inode
    char *s,*e;
    s=e=kernel;
    i=0;
again:
    while(*e!='/'&&*e!=0){e++;}
    if(*e=='/'){e++;}
    if(!memcmp(in->magic,FSZ_IN_MAGIC,4)){
        //is it inlined?
        if(!memcmp(sb->flags&FSZ_SB_FLAG_BIGINODE? in->data.big.inlinedata : in->data.small.inlinedata,FSZ_DIR_MAGIC,4)){
            ent=(FSZ_DirEnt *)(sb->flags&FSZ_SB_FLAG_BIGINODE? in->data.big.inlinedata : in->data.small.inlinedata);
        } else if(!memcmp(initrd_p+in->sec*ss,FSZ_DIR_MAGIC,4)){
            // go, get the sector pointed
            ent=(FSZ_DirEnt *)(initrd_p+in->sec*ss);
        } else {
            return ret;
        }
        //skip header
        FSZ_DirEntHeader *hdr=(FSZ_DirEntHeader *)ent; ent++;
        //iterate on directory entries
        int j=hdr->numentries;
        while(j-->0){
            if(!memcmp(ent->name,s,e-s)) {
                if(*e==0) {
                    i=ent->fid;
                    break;
                } else {
                    s=e;
                    in=(FSZ_Inode *)(initrd_p+ent->fid*ss);
                    goto again;
                }
            }
            ent++;
        }
    } else {
        i=0;
    }
    if(i!=0) {
        // fid -> inode ptr -> data ptr
        FSZ_Inode *in=(FSZ_Inode *)(initrd_p+i*ss);
        if(!memcmp(in->magic,FSZ_IN_MAGIC,4)){
            ret.size=in->size;
            switch(FSZ_FLAG_TRANSLATION(in->flags)) {
                case FSZ_IN_FLAG_INLINE:
                    // inline data
                    ret.ptr=(uint8_t*)(initrd_p+i*ss+1024);
                    break;
                case FSZ_IN_FLAG_SECLIST:
                case FSZ_IN_FLAG_SDINLINE:
                    // sector directory or list inlined
                    ret.ptr=(uint8_t*)(initrd_p + *(sb->flags&FSZ_SB_FLAG_BIGINODE?
                        (uint64_t*)&in->data.big.inlinedata : (uint64_t*)&in->data.small.inlinedata) * ss);
                    break;
                case FSZ_IN_FLAG_DIRECT:
                    // direct data
                    ret.ptr=(uint8_t*)(initrd_p + in->sec * ss);
                    break;
                // sector directory (only one level supported here, and no holes in files)
                case FSZ_IN_FLAG_SECLIST0:
                case FSZ_IN_FLAG_SD:
                    ret.ptr=(uint8_t*)(initrd_p + (unsigned int)(((FSZ_SectorList *)(initrd_p+in->sec*ss))->sec) * ss);
                    break;
                default:
                    ret.size=0;
                    break;
            }
        }
    }
    return ret;
}
#endif

/**
 * cpio archive
 */
file_t cpio_initrd(unsigned char *initrd_p, char *kernel)
{
    unsigned char *ptr=initrd_p;
    int k;
    file_t ret = { NULL, 0 };
    if(initrd_p==NULL || kernel==NULL ||
        (memcmp(initrd_p,"070701",6) && memcmp(initrd_p,"070702",6) && memcmp(initrd_p,"070707",6)))
        return ret;
    DBG(" * cpio ");
    DBG(kernel);
    DBG("\n");
    k=strlen((unsigned char*)kernel);
    // hpodc archive
    while(!memcmp(ptr,"070707",6)){
        int ns=oct2bin(ptr+8*6+11,6);
        int fs=oct2bin(ptr+8*6+11+6,11);
        if(!memcmp(ptr+9*6+2*11,kernel,k+1)){
            ret.size=fs;
            ret.ptr=(uint8_t*)(ptr+9*6+2*11+ns);
            return ret;
        }
        ptr+=(76+ns+fs);
    }
    // newc and crc archive
    while(!memcmp(ptr,"07070",5)){
        int fs=hex2bin(ptr+8*6+6,8);
        int ns=hex2bin(ptr+8*11+6,8);
        if(!memcmp(ptr+110,kernel,k+1)){
            ret.size=fs;
            ret.ptr=(uint8_t*)(ptr+((110+ns+3)/4)*4);
            return ret;
        }
        ptr+=((110+ns+3)/4)*4 + ((fs+3)/4)*4;
    }
    return ret;
}

/**
 * ustar tarball archive
 */
file_t tar_initrd(unsigned char *initrd_p, char *kernel)
{
    unsigned char *ptr=initrd_p;
    int k;
    file_t ret = { NULL, 0 };
    if(initrd_p==NULL || kernel==NULL || memcmp(initrd_p+257,"ustar",5))
        return ret;
    DBG(" * tar ");
    DBG(kernel);
    DBG("\n");
    k=strlen((unsigned char*)kernel);
    while(!memcmp(ptr+257,"ustar",5)){
        int fs=oct2bin(ptr+0x7c,11);
        if(!memcmp(ptr,kernel,k+1)){
            ret.size=fs;
            ret.ptr=(uint8_t*)(ptr+512);
            return ret;
        }
        ptr+=(((fs+511)/512)+1)*512;
    }
    return ret;
}

/**
 * Simple File System
 */
file_t sfs_initrd(unsigned char *initrd_p, char *kernel)
{
    unsigned char *ptr, *end;
    int k,bs,ver;
    file_t ret = { NULL, 0 };
    if(initrd_p==NULL || kernel==NULL || (memcmp(initrd_p+0x1AC,"SFS",3) && memcmp(initrd_p+0x1A6,"SFS",3)))
        return ret;
    // 1.0 Brendan's version, 1.10 BenLunt's version
    ver=!memcmp(initrd_p+0x1A6,"SFS",3)?10:0;
    bs=1<<(7+(uint8_t)initrd_p[ver?0x1B6:0x1BC]);
    end=initrd_p + *((uint64_t *)&initrd_p[ver?0x1AA:0x1B0]) * bs; // base + total_number_of_blocks * blocksize
    // get index area
    ptr=end - *((uint64_t *)&initrd_p[ver?0x19E:0x1A4]); // end - size of index area
    // got a Starting Marker Entry?
    if(ptr[0]!=2)
        return ret;
    DBG(" * SFS 1.");
    DBG(ver?"10":"0");
    DBG(kernel);
    DBG("\n");
    k=strlen((unsigned char*)kernel);
    // iterate on index until we reach the end or Volume Identifier
    while(ptr<end && ptr[0]!=0x01){
        ptr+=64;
        // file entry?
        if(ptr[0]!=0x12)
            continue;
        // filename match?
        if(!memcmp(ptr+(ver?0x23:0x22),kernel,k+1)){
            ret.size=*((uint64_t*)&ptr[ver?0x1B:0x1A]);                 // file_length
            ret.ptr=initrd_p + *((uint64_t*)&ptr[ver?0x0B:0x0A]) * bs; // base + start_block * blocksize
            break;
        }
    }
    return ret;
}

/**
 * James Molloy's initrd (for some reason it's popular among hobby OS developers)
 * http://www.jamesmolloy.co.uk/tutorial_html
 */
file_t jamesm_initrd(unsigned char *initrd_p, char *kernel)
{
    unsigned char *ptr=initrd_p+4;
    int i,k,nf=*((int*)initrd_p);
    file_t ret = { NULL, 0 };
    // no real magic, so we assume initrd contains at least 2 files...
    if(initrd_p==NULL || kernel==NULL || initrd_p[2]!=0 || initrd_p[3]!=0 || initrd_p[4]!=0xBF || initrd_p[77]!=0xBF)
        return ret;
    DBG(" * JamesM ");
    DBG(kernel);
    DBG("\n");
    k=strlen((unsigned char*)kernel);
    for(i=0;i<nf && ptr[0]==0xBF;i++) {
        if(!memcmp(ptr+1,kernel,k+1)){
            ret.ptr=*((uint32_t*)(ptr+65)) + initrd_p;
            ret.size=*((uint32_t*)(ptr+69));
        }
        ptr+=73;
    }
    return ret;
}

/**
 * Static file system drivers registry
 */
file_t (*fsdrivers[]) (unsigned char *, char *) = {
#ifdef _FS_Z_H_
    fsz_initrd,
#endif
    cpio_initrd,
    tar_initrd,
    sfs_initrd,
    jamesm_initrd,
    NULL
};
