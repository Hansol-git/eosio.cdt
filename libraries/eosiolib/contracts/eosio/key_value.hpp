#include "../../core/eosio/name.hpp"

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
   };

template<typename T>
class kv_table {
   struct iterator {
      T data;
      uint32_t data_size;
      std::string _key;

      // TODO: How to get db name and contract name to iterator?
      eosio::name table_name;
      eosio::name contract_name;
      // TODO:

      iterator(T data, std::string key): data{data}, _key{key} {}

      T value() {
         uint32_t offset = 0; // TODO??
         char* d;
         uint32_t copy_size = internal_use_do_not_use::kv_get_data(table_name.value(), 0, d, data_size);

         // TODO: Cast back to the appropriate type
      }

      // TODO:
      std::string key() {
      }

      iterator operator++() {
      }

      iterator operator--() {
      }
   };

public:
   struct index {
      eosio::name name;
      std::string (T::*key_function)() const;
      // std::function<std::string(T*)>* key_function;

      // TODO: How to get db name and contract name to index?
      eosio::name table_name;
      eosio::name contract_name;
      // TODO:

      index(eosio::name name, std::string (T::*key_function)() const): name{name}, key_function{key_function} {}

      iterator find(std::string key) {
         uint32_t* value_size;
         auto success = internal_use_do_not_use::kv_get(table_name.value(), contract_name.value(), key.c_str(), key.size(), value_size);
         if (!success) {
            return end();
         }

         // TODO: call kv_it_create
         // TODO: create iterator
      }

      iterator end() {
         uint32_t itr = internal_use_do_not_use::kv_it_create(table_name.value(), contract_name.value(), "", 0);
         internal_use_do_not_use::kv_it_move_to_end(itr);
      }

      iterator begin() {
      }

      std::vector<T> range(std::string b, std::string e) {
         auto begin_itr = find(b);
         auto end_itr = find(e);
         eosio::check(begin_itr != end(), "beginning of range is not in table");
         eosio::check(end_itr != end(), "end of range is not in table");
      }

   };

   // TODO: Is db param == to table_name.value()?



   kv_table(eosio::name contract_name, eosio::name table_name): contract_name{contract_name}, table_name{table_name} {}

   void upsert(T value) {
      std::string key = value.primary_key();
      char* data;

      memcpy(data, &value, sizeof(value));

      internal_use_do_not_use::kv_set(table_name.value(), contract_name.value(), key.c_str(), key.size(), data, sizeof(value));
   }

   void erase(std::string key) {
      internal_use_do_not_use::kv_erase(table_name.value(), contract_name.value(), key.c_str(), key.size());
   }
private:
   eosio::name contract_name;
   eosio::name table_name;


};
} // eosio
