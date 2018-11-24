#ifndef PIVOT_NEIGHBORHOOD_HORDER_COMMON_H
#define PIVOT_NEIGHBORHOOD_HORDER_COMMON_H

typedef uint32_t PostingListHorderElemType;
typedef std::vector<PostingListHorderElemType> PostingListHorderType;

/**
 * A structure that keeps information about current state of search within one posting list.
 */
struct PostListQueryState {
  // pointer to the posting list (fixed from the beginning)
  const PostingListHorderType&  post_;
  // actual position in the list
  unsigned                post_pos_;

  PostListQueryState(const PostingListHorderType& pl) :
    post_(pl), post_pos_(0) {}
};

#endif