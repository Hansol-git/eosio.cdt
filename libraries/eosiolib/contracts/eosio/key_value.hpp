#pragma once
#include "../../core/eosio/datastream.hpp"
#include "../../core/eosio/name.hpp"
#include "../../core/eosio/print.hpp"

#include <functional>

namespace eosio {
   namespace internal_use_do_not_use {
      extern "C" {
         __attribute__((eosio_wasm_import))
         void kv_erase(uint64_t db, uint64_t contract, const char* key, uint32_t key_size);

         __attribute__((eosio_wasm_import))
         void kv_set(uint64_t db, uint64_t contract, const char* key, uint32_t key_size, const char* value, uint32_t value_size);

         __attribute__((eosio_wasm_import))
         bool kv_get(uint64_t db, uint64_t contract, const char* key, uint32_t key_size, uint32_t& value_size);

         __attribute__((eosio_wasm_import))
         uint32_t kv_get_data(uint64_t db, uint32_t offset, char* data, uint32_t data_size);

         __attribute__((eosio_wasm_import))
         uint32_t kv_it_create(uint64_t db, uint64_t contract, const char* prefix, uint32_t size);

         __attribute__((eosio_wasm_import))
         void kv_it_destroy(uint32_t itr);

         __attribute__((eosio_wasm_import))
         int32_t kv_it_status(uint32_t itr);

         __attribute__((eosio_wasm_import))
         int32_t kv_it_compare(uint32_t itr_a, uint32_t itr_b);

         __attribute__((eosio_wasm_import))
         int32_t kv_it_key_compare(uint32_t itr, const char* key, uint32_t size);

         __attribute__((eosio_wasm_import))
         int32_t kv_it_move_to_end(uint32_t itr);

         __attribute__((eosio_wasm_import))
         int32_t kv_it_next(uint32_t itr);

         __attribute__((eosio_wasm_import))
         int32_t kv_it_prev(uint32_t itr);

         __attribute__((eosio_wasm_import))
         int32_t kv_it_lower_bound(uint32_t itr, const char* key, uint32_t size);

         __attribute__((eosio_wasm_import))
         int32_t kv_it_key(uint32_t itr, uint32_t offset, char* dest, uint32_t size, uint32_t& actual_size);

         __attribute__((eosio_wasm_import))
         int32_t kv_it_value(uint32_t itr, uint32_t offset, char* dest, uint32_t size, uint32_t& actual_size);
      }
   }

// This won't work with multiple indexes
template<typename T, typename K>
class kv_table {
   constexpr static size_t max_stack_buffer_size = 512;
   constexpr static uint64_t db = eosio::name{"eosio.kvram"}.value;

   enum class kv_it_stat {
      iterator_ok     = 0,  // Iterator is positioned at a key-value pair
      iterator_erased = -1, // The key-value pair that the iterator used to be positioned at was erased
      iterator_end    = -2, // Iterator is out-of-bounds
   };

   struct iterator {
      uint32_t itr;
      kv_it_stat itr_stat;

      uint32_t data_size;

      K _key; // Make sure this doesn't get out of sync
      T data; // TODO: Is this needed?

      iterator(uint32_t itr, kv_it_stat itr_stat): itr{itr}, itr_stat{itr_stat} {}
      iterator(uint32_t itr, kv_it_stat itr_stat, uint32_t data_size): itr{itr}, itr_stat{itr_stat}, data_size{data_size} {}
      iterator(uint32_t itr, kv_it_stat itr_stat, K _key, T data): itr{itr}, itr_stat{itr_stat}, _key{_key}, data{data} {}

      const T value() {
         uint32_t actual_size;
         uint32_t offset = 0; // TODO??

         void* buffer = max_stack_buffer_size < size_t(data_size) ? malloc(size_t(data_size)) : alloca(size_t(data_size));

         // TODO: Can't use kv_it_value??
         internal_use_do_not_use::kv_it_value(itr, offset, (char*)buffer, data_size, actual_size);

         datastream<const char*> ds( (char*)buffer, data_size);

         T val;
         ds >> val;

         return val;
      }

      const K key() {
         size_t k_size = sizeof(K);
         void* buffer = max_stack_buffer_size < k_size ? malloc(k_size) : alloca(k_size);

         uint32_t copy_size;
         internal_use_do_not_use::kv_it_key(itr, 0, (char*)buffer, k_size, copy_size);

         datastream<const char*> ds( (char*)buffer, copy_size);
         K key;
         ds >> key;
         _key = key;

         return _key;
      }

      iterator operator++() {
         itr_stat = internal_use_do_not_use::kv_it_next(itr);
         return this; // TODO
      }

      iterator operator--() {
         itr_stat = internal_use_do_not_use::kv_it_prev(itr);
         return this; // TODO
      }

      bool operator==(iterator b) {
         return _key == b._key;
      }
      bool operator!=(iterator b) {
         return _key != b._key;
      }
   };

public:
   struct index {
      eosio::name name;
      K (T::*key_function)() const;

      std::vector<uint32_t> iterators;

      eosio::name contract_name;

      // TODO: We need a way to store different indexes. See NOTES.md for one possible approach to encoding it in the key.
      // Would need a function to help us build those keys.

      index() = default;

      index(eosio::name name, K (T::*key_function)() const): name{name}, key_function{key_function} {}

      iterator find(K key) {
         uint32_t value_size;

         size_t key_size = pack_size( key );
         void* key_buffer = max_stack_buffer_size < key_size ? malloc(key_size) : alloca(key_size);
         datastream<char*> key_ds( (char*)key_buffer, key_size );
         key_ds << key;

         eosio::print_f("\n\nfind %\n", key);

         auto success = internal_use_do_not_use::kv_get(db, contract_name.value, (const char*)key_buffer, key_size, value_size);
         if (!success) {
            return end();
         }

         eosio::print_f("success %\n", success);

         if (iterators.size() == 0) {
            uint32_t itr = internal_use_do_not_use::kv_it_create(db, contract_name.value, "", 0); // TODO: prefix
            iterators.push_back(itr);
         }
         uint32_t itr = iterators[iterators.size() - 1];

         kv_it_stat itr_stat = internal_use_do_not_use::kv_it_lower_bound(itr, (const char*) key_buffer, key_size);
         auto cmp = internal_use_do_not_use::kv_it_key_compare(itr, (const char*)key_buffer, key_size);
         eosio::check(cmp == 0, "why did this fail");

         return {itr, itr_stat, value_size};
      }

      iterator end() {
         uint32_t itr = internal_use_do_not_use::kv_it_create(db, contract_name.value, "", 0); // TODO: prefix
         int32_t itr_stat = internal_use_do_not_use::kv_it_move_to_end(itr);

         return {itr, static_cast<kv_it_stat>(itr_stat)};
      }

      iterator begin() {
         uint32_t itr = internal_use_do_not_use::kv_it_create(db, contract_name.value, "", 0); // TODO: prefix
         int32_t itr_stat = internal_use_do_not_use::kv_it_lower_bound(itr, "", 0); // TODO: prefix

         return {itr, static_cast<kv_it_stat>(itr_stat)};
      }

      // TODO
      std::vector<T> range(std::string b, std::string e) {
         auto begin_itr = find(b);
         auto end_itr = find(e);
         eosio::check(begin_itr != end(), "beginning of range is not in table");
         eosio::check(end_itr != end(), "end of range is not in table");

         // Get the beginning itr
         // Advance through and get the data for each next iterator until we hit end iterator.
         // Is end iterator inclusive?
      }
   };

   kv_table() = default;

   void init(eosio::name contract, eosio::name table, index* primary) {
      contract_name = contract;
      table_name = table;

      // TODO Temporary, need support for multiple indexes
      primary_index = primary;
      primary_index->contract_name = contract;
   }

   void upsert(T value) {
      K key = value.primary_key();

      size_t data_size = pack_size( value );
      void* data_buffer = max_stack_buffer_size < data_size ? malloc(data_size) : alloca(data_size);
      datastream<char*> data_ds( (char*)data_buffer, data_size );
      data_ds << value;

      size_t key_size = pack_size( key );
      void* key_buffer = max_stack_buffer_size < key_size ? malloc(key_size) : alloca(key_size);
      datastream<char*> key_ds( (char*)key_buffer, key_size );
      key_ds << key;

      internal_use_do_not_use::kv_set(db, contract_name.value, (const char*)key_buffer, key_size, (const char*)data_buffer, data_size);
   }

   // May make sense to move this to an index.
   // Then the table doesn't need to know anything about the indexes.
   void erase(K key) {
      size_t key_size = pack_size( key );
      void* key_buffer = max_stack_buffer_size < key_size ? malloc(key_size) : alloca(key_size);
      datastream<char*> key_ds( (char*)key_buffer, key_size );
      key_ds << key;

      auto itr = primary_index->find(key);

      internal_use_do_not_use::kv_erase(db, contract_name.value, (const char*)key_buffer, key_size);
   }

private:
   eosio::name contract_name;
   eosio::name table_name;
   index* primary_index; // TODO: Temporary
};
} // eosio
