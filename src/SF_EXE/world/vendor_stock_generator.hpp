#ifndef OPENSHADOWFLARE_VENDOR_STOCK_GENERATOR_HPP
#define OPENSHADOWFLARE_VENDOR_STOCK_GENERATOR_HPP

#include <cstdint>

namespace osf {

class ItemDatabase;
class RetailRandom;
class TableDatabase;
class VendorInventory;

bool generateRetailVendorStock(
    VendorInventory& inventory,
    std::int32_t profile,
    const TableDatabase& tables,
    const ItemDatabase& items,
    RetailRandom& random);

}  // namespace osf

#endif
