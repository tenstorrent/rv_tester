// SPDX-FileCopyrightText: 2026 Tenstorrent USA, Inc.
// SPDX-License-Identifier: Apache-2.0

// Compile-and-sanity test for the generated CSRAL tables (csral_tables.hpp,
// emitted by csr_param_gen alongside csr_param.hpp). Most guarantees are
// static_asserts: they hold for the sw_1c generation of the default spec +
// project overrides, at compile time.

#include <gtest/gtest.h>

#include "csr_param.hpp"
#include "csral_tables.hpp"

namespace {

static_assert(CSRAL::kNumCsrs > 0, "tables must not be empty");
static_assert(CSRAL::kCsrs.size() == CSRAL::kNumCsrs);
static_assert(CSRAL::kFields.size() == CSRAL::kNumFields);

// The O(1) address index agrees with the table rows.
static_assert(CSRAL::find_by_address(0x300) != nullptr);
static_assert(CSRAL::find_by_address(0x300)->name == "mstatus");
static_assert(CSRAL::find_by_address(0x301)->name == "misa");
static_assert(CSRAL::find_by_address(0x344)->name == "mip");
static_assert(CSRAL::find_by_address(0x5555) == nullptr);

// Field helpers round-trip.
constexpr CSRAL::field_t kTestField{"T", 11, 4, 0xFF0, 0, 0, 0, ""};
static_assert(CSRAL::field_extract(kTestField, 0xAB0) == 0xAB);
static_assert(CSRAL::field_insert(kTestField, 0xF00F, 0xAB) == 0xFABF);
static_assert(CSRAL::field_insert(kTestField, 0x0, 0xAB) == 0xAB0);

TEST(CsralTablesTest, AddressIndexMatchesLegacyMap) {
  // Every directly-addressed CSR in the legacy csr_map is findable in the
  // CSRAL tables under the same name, and vice versa.
  for (const auto* legacy : CSR::csr_map) {
    const auto* row = CSRAL::find_by_address(legacy->address);
    ASSERT_NE(row, nullptr) << legacy->name;
    EXPECT_EQ(row->name, legacy->name);
  }
}

TEST(CsralTablesTest, ResetsMatchLegacyHeader) {
  for (const auto* legacy : CSR::csr_map) {
    const auto* row = CSRAL::find_by_address(legacy->address);
    ASSERT_NE(row, nullptr) << legacy->name;
    EXPECT_EQ(row->reset, legacy->reset_val) << legacy->name;
  }
}

TEST(CsralTablesTest, PoliciesReflectCacCheck) {
  // cac_check in the legacy header and CSRAL's policy.check come from the
  // same YAML inputs; they must agree.
  for (const auto* legacy : CSR::csr_map) {
    const auto* row = CSRAL::find_by_address(legacy->address);
    ASSERT_NE(row, nullptr) << legacy->name;
    EXPECT_EQ(row->policy.check, legacy->cac_check) << legacy->name;
  }
}

TEST(CsralTablesTest, MisaHConditionIsDerived) {
  bool found = false;
  for (std::size_t i = 0; i < CSRAL::kNumConditions; ++i) {
    const auto& c = CSRAL::kConditions[i];
    if (c.name == "misa.H") {
      found = true;
      EXPECT_EQ(CSRAL::kCsrs[c.gate_csr].name, "misa");
      EXPECT_EQ(c.gate_mask, 0x80u);
      EXPECT_FALSE(c.view_only);
      // The spec-derived mstatus mask is MPV|GVA; bridge.h's hand literal
      // (0x0000000300000000, which is UXL) is a known bug this replaces.
      bool saw_mstatus = false;
      for (std::size_t t = 0; t < c.target_count; ++t) {
        const auto& tgt = CSRAL::kMaskedTargets[c.target_first + t];
        if (CSRAL::kCsrs[tgt.csr].name == "mstatus") {
          saw_mstatus = true;
          EXPECT_EQ(tgt.mask, 0x000000C000000000ull);
        }
      }
      EXPECT_TRUE(saw_mstatus);
    }
  }
  EXPECT_TRUE(found);
}

} // namespace
