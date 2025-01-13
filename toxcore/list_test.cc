#include "list.h"

#include <gtest/gtest.h>

#include "mem_test_util.hh"

namespace {

TEST(List, CreateAndDestroyWithNonZeroSize)
{
    Test_Memory mem;
    BS_List list;
    bs_list_init(&list, mem, sizeof(int), 10, memcmp);
    bs_list_free(&list);
}

TEST(List, CreateAndDestroyWithZeroSize)
{
    Test_Memory mem;
    BS_List list;
    bs_list_init(&list, mem, sizeof(int), 0, memcmp);
    bs_list_free(&list);
}

TEST(List, DeleteFromEmptyList)
{
    Test_Memory mem;
    BS_List list;
    bs_list_init(&list, mem, sizeof(int), 0, memcmp);
    const uint8_t data[sizeof(int)] = {0};
    bs_list_remove(&list, data, 0);
    bs_list_free(&list);
}

}  // namespace
