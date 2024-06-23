#include "os.h"

/* Constants: */

/* Similar to the calculation we saw in the lecture, for the sizes in the trie (the data-structure that implements the mapping) */
#define Frame_Size 4096 * 8           /* Is given in the assignment instructions (4 KB / 4096 bytes) */
#define PTE_Size 64                   /* Is given in the assignment instructions */
#define Trie_Table_Size 4096 * 8      /* = Frame Size */
#define PTEs_in_Trie_Table 512        /* = (Trie-Table Size) / (PTE Size) = (4096 * 8) / 64 = 512 */
#define Symbol_Size 9                 /* = Log2(PTEs in a Trie-Table) = log2(512) = 9 */
#define Layers_amount_in_Trie_Table 5 /* = (Address Size) / (Symbol size) = 45 / 9 = 5 */

#define EXT_9_MSB 0x1FF /* = 111111111 in binary (Mask to the 9 least significant bits) */
#define Off_Size 12     /* Physical Page Adress Offset Size */

/* Main Functions: */

/* A function to create/destroy virtual memory mappings in a page table.
   Args description:
   (a) pt: The physical page number of the page table root.
   (b) vpn: The virtual page number the caller wishes to map/unmap.
   (c) ppn:
        ppn = NO MAPPING: vpn’s mapping (if it exists) should be destroyed.
        Otherwise: ppn specifies the ppn that vpn should be mapped to. */
void page_table_update(uint64_t pt, uint64_t vpn, uint64_t ppn);

/* A function to query the mapping of a VPN in a page table. Returns the PPN that VPN is mapped to, or NO MAPPING if no mapping exists. */
uint64_t page_table_query(uint64_t pt, uint64_t vpn);

/* Helpers */

uint64_t get_table_phys_addr(int layer, uint64_t pt, uint64_t pte);
int get_symbol(uint64_t vpn, int layer);
int get_valid_bit(uint64_t pte);
void vpn_mapping_destroy(uint64_t *table, int symbol);
uint64_t set_valid_bit(uint64_t address, int val);
void alloc_page_or_point2ppn_and_update_in_table(uint64_t *table, int symbol, int layer, uint64_t orig_ppn);

/* Implementation */

/* Returns PT if you want to access the trie`s root, otherwise the last PTE we found  */
uint64_t get_table_phys_addr(int layer, uint64_t pt, uint64_t pte)
{
    /* pt:= PPN of the PT-root. Was returned by 'alloc_page_frame' -> pt is a PPN (PA without PPO:=PA`s 12 lsb).
       Therefore: layer = 0 iff next_table_address corresponding to the PPN of the PT-root. */
    if (layer == 0)
        return (pt << Off_Size); /* Adding offset (zero-flags) of 12 lsb to the ppn of the root */
    /* Otherwise: PTE`s value is a pointer to the corresponding node in the next layer of the trie (a node frame #). */
    return pte;
}

/* Each sequence of 9 bits in VPN will be a symbol according to which we will go through the layers of the trie. In layer 0: the 9 msb are taken, and in layer 4, the 9 lsb are taken (vpn is 45 bits) */
int get_symbol(uint64_t vpn, int layer)
{
    if (layer == 0)
        return ((vpn >> 36) & EXT_9_MSB);
    if (layer == 1)
        return ((vpn >> 27) & EXT_9_MSB);
    if (layer == 2)
        return ((vpn >> 18) & EXT_9_MSB);
    if (layer == 3)
        return ((vpn >> 9) & EXT_9_MSB);
    if (layer == 4)
        return (vpn & EXT_9_MSB);
    return 0;
}

/* Returns the lsb of the address; An indicator for if the key we are looking for is in the table, or not */
int get_valid_bit(uint64_t pte)
{
    return pte & 0x1;
}

/* Zero the current node in the table */
void vpn_mapping_destroy(uint64_t *table, int symbol)
{
    table[symbol] = 0;
}

/* Returns address s.t. it`s 0 bit equals to val (0 or 1) */
uint64_t set_valid_bit(uint64_t address, int val)
{
    if (val == 1)
        return address + 1;
    return address - 1;
}

/* Updates the phys-address in the current node in the table to the address of the new frame we allocated / to the given ppn if we are in the leaf-layer of the trie */
void alloc_page_or_point2ppn_and_update_in_table(uint64_t *table, int symbol, int layer, uint64_t orig_ppn)
{
    uint64_t ppn, phys_add;
    if (layer != (Layers_amount_in_Trie_Table - 1))
        ppn = alloc_page_frame();
    else
        ppn = orig_ppn;
    phys_add = ppn << Off_Size;
    phys_add = set_valid_bit(phys_add, 1);
    table[symbol] = phys_add;
}

/* A function to create/destroy virtual memory mappings in a page table */
void page_table_update(uint64_t pt, uint64_t vpn, uint64_t ppn)
{
    int layer, depth, valid, symbol;
    uint64_t table_phys_addr, *table, pte;

    layer = 0;
    depth = Layers_amount_in_Trie_Table;

    while (layer < depth) /* Page-Walk in order to create/destroy virtual memory mappings in the page table */
    {
        table_phys_addr = get_table_phys_addr(layer, pt, pte);
        if (layer != 0)
            table_phys_addr = set_valid_bit(table_phys_addr, 0); /* phys_to_virt gets an phys-add with 12-lsb = 0 */
        table = phys_to_virt(table_phys_addr);
        symbol = get_symbol(vpn, layer); /* 9 bit seq of the given VPN, its indeces are determined by the curr trie-layer */
        pte = table[symbol]; /* The node in the index of the curr symbol determines where to move in our next step */
        valid = get_valid_bit(pte);

        if (layer == (depth - 1)) /* The leaf-layer */
        {
            if (ppn == NO_MAPPING)
            {
                vpn_mapping_destroy(table, symbol); /* If ppn is equal to NO_MAPPING, then vpn’s mapping (that exists - we are in the last layer of the trie) should be destroyed */
                return;
            }
            else
                alloc_page_or_point2ppn_and_update_in_table(table, symbol, layer, ppn); /* If ppn isn`t equal to NO_MAPPING, ppn specifies the physical page number that vpn should be mapped to */
            return;
        }

        if (valid == 0)
        {
            if (ppn == NO_MAPPING) /* If ppn is equal to NO_MAPPING, then vpn’s mapping should be destroyed - but Valid_bit = 0 -> does not exist */
                return;
            alloc_page_or_point2ppn_and_update_in_table(table, symbol, layer, ppn); /* Valid_bit = 0 -> the key we were looking for isn`t in the table, so we will allocate a page in order to create the mapping */
            pte = table[symbol];
        }

        layer += 1; /* Towards the next trie-layer */
    }
    return;
}

/* A function to query the mapping of a virtual page number in a page table */
uint64_t page_table_query(uint64_t pt, uint64_t vpn)
{
    int layer, depth, valid, symbol;
    uint64_t table_phys_addr, *table, pte;

    layer = 0;
    depth = Layers_amount_in_Trie_Table;

    while (layer < depth) /* Page-Walk towards the leave which represents the key and stores the desired PTE if mapping exists */
    {
        table_phys_addr = get_table_phys_addr(layer, pt, pte);
        if (layer != 0)
            table_phys_addr = set_valid_bit(table_phys_addr, 0); /* phys_to_virt gets an phys-add with 12-lsb = 0 */
        table = phys_to_virt(table_phys_addr);
        symbol = get_symbol(vpn, layer); /* 9 bit seq of the given VPN, its indeces are determined by the curr trie-layer */
        pte = table[symbol]; /* The node in the index of the curr symbol determines where to move in our next step */
        valid = get_valid_bit(pte);

        if (valid == 0)
            return NO_MAPPING; /* Valid_bit = 0 -> Fault, the key we were looking for isn`t in the table */

        layer += 1; /* Towards the next trie-layer */
    }

    return (pte >> Off_Size); /* Reached to the the leaf layer, and the valid bit of table[symbol] is 1 -> found the PPN; PPN in bits 12-63 of PTE */
}