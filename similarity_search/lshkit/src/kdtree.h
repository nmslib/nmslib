#ifndef WDONG_KD_TREE
#define WDONG_KD_TREE

#ifdef __cplusplus
extern "C" {
#endif

typedef struct kd_tree kd_tree_t;

kd_tree_t *kd_tree_alloc (unsigned K, unsigned dim);

void kd_tree_free (kd_tree_t *tree);

void kd_tree_index (kd_tree_t *tree, const float *means);

unsigned kd_tree_search (kd_tree_t *tree, const float *pt, unsigned *cnt);

unsigned kd_tree_ln_search (kd_tree_t *tree, const float *pt, unsigned *cnt);

#ifdef __cplusplus
}
#endif

#endif

