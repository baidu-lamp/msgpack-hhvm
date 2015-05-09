/*
   +----------------------------------------------------------------------+
   | HipHop for PHP                                                       |
   +----------------------------------------------------------------------+
   | Copyright (c) 2014-2015 Baidu, Inc.                                  |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
*/

// HHVM Variant to msgpack implementation

#ifndef MSGPACK_TYPE_VARIANT_H__
#define MSGPACK_TYPE_VARIANT_H__

#include "hphp/runtime/ext/extension.h"
#include "hphp/runtime/base/array-iterator.h"

#include <msgpack.hpp>

using namespace HPHP;

namespace msgpack {


// Unpack
inline Variant& operator>> (object o, Variant& v)
{
  switch(o.type) {
    case type::NIL:
      v.setNull();
      break;
    case type::BOOLEAN:
      v = o.via.boolean ? true : false;
      break;
    case type::POSITIVE_INTEGER:
      v = (int64_t) o.via.u64;
      break;
    case type::NEGATIVE_INTEGER:
      v = (int64_t) o.via.i64;
      break;
    case type::DOUBLE:
      v = o.via.dec;
      break;
    case type::RAW: {
      if (o.via.raw.size == 0) {
        v = String("");
      } else {
        v = String((char*)o.via.raw.ptr, o.via.raw.size, CopyString);
      }
      break;
    }
    case type::ARRAY: {
      Array array = Array::Create();
      if(o.via.array.size != 0) {
        object* p(o.via.array.ptr);
        for(object* const pend(o.via.array.ptr + o.via.array.size);
            p < pend; ++p) {
          Variant item;
          Variant& item_ref = item;
          *p >> item;
          array.append(item_ref);
        }
      }

      v = array;
      break;
    }
    case type::MAP: {
      Array array = Array::Create();
      if(o.via.map.size != 0) {
        object_kv* p(o.via.map.ptr);
        for(object_kv* const pend(o.via.map.ptr + o.via.map.size);
          p < pend; ++p) {
          Variant key, val;
          p->key >> key;
          p->val >> val;
          array.set(key, val, true);
        }
      }
      v = array;
      break;
    }
    default:
      break;
  }

  return v;
}


template <typename Stream>
inline packer<Stream>& pack_variant(packer<Stream>& o, const Variant& v, int depth=0)
{
  if (depth > 512) {
    raise_warning("Nested too deep, greate than 512");
    return o;
  }

  ++depth;

  switch (v.getType()) {
    case KindOfUninit:
    case KindOfNull:
      o.pack_nil();
      break;
    case KindOfBoolean:
      if (v.asBooleanVal()) {
        o.pack_true();
      } else {
        o.pack_false();
      }
      break;
    case KindOfInt64:
      o.pack_int64(v.asInt64Val());
      break;
    case KindOfDouble:
      o.pack_double(v.asDoubleVal());
      break;
    case KindOfStaticString:
    case KindOfString: {
      String str = v.toString();
      o.pack_raw(str.size());
      o.pack_raw_body(str.c_str(), str.size());
      break;
    }
    case KindOfArray: {
      Array array = v.toArray();
      if (array->isVectorData()) {
        o.pack_array(array.size());
        for (ArrayIter iter(array); iter; ++iter) {
          pack_variant(o, iter.second(), depth);
        }
      } else {
        o.pack_map(array.size());
        for (ArrayIter iter(array); iter; ++iter) {
          pack_variant(o, iter.first(), depth);
          pack_variant(o, iter.second(), depth);
        }
      }
      break;
    }
    default:
      raise_warning("Not implemented source type");
      break;
  }

  return o;
}

// Pack
template <typename Stream>
inline packer<Stream>& operator<< (packer<Stream>& o, const Variant& v)
{
  return pack_variant(o, v);
}

}  // namespace msgpack

#endif
