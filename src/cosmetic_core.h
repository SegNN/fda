#pragma once
// Local-only inventory projection. No sockets, Steam calls, ownership writes or engine offsets.
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <deque>
#include <stdexcept>
#include <algorithm>
#include <limits>
namespace cosmetic {
using Bytes=std::vector<uint8_t>;
struct Definition {uint32_t id,cls,slot,quality;const char *styles,*name,*hero,*slotName,*rarity;};
namespace pb {
constexpr size_t MaxBytes=16*1024*1024,MaxFields=100000;
struct Field {uint32_t n=0;uint8_t wire=0;uint64_t value=0;Bytes bytes;};
using Message=std::vector<Field>;
inline uint64_t ReadVar(const Bytes& b,size_t& pos){uint64_t v=0;for(unsigned i=0;i<10;++i){if(pos==b.size())throw std::runtime_error("truncated varint");uint8_t c=b[pos++];if(i==9&&c>1)throw std::runtime_error("varint overflow");v|=uint64_t(c&127)<<(i*7);if(!(c&128))return v;}throw std::runtime_error("bad varint");}
inline void Var(Bytes& b,uint64_t v){while(v>127){b.push_back(uint8_t(v)|128);v>>=7;}b.push_back(uint8_t(v));}
inline Message Decode(const Bytes& b){if(b.size()>MaxBytes)throw std::runtime_error("size limit");Message out;size_t p=0;while(p<b.size()){if(out.size()>=MaxFields)throw std::runtime_error("field limit");uint64_t tag=ReadVar(b,p);if((tag>>3)==0||(tag>>3)>0x1fffffff)throw std::runtime_error("bad tag");Field f;f.n=uint32_t(tag>>3);f.wire=uint8_t(tag&7);if(f.wire==0)f.value=ReadVar(b,p);else if(f.wire==1||f.wire==5){size_t n=f.wire==1?8:4;if(n>b.size()-p)throw std::runtime_error("fixed truncated");for(size_t i=0;i<n;++i)f.value|=uint64_t(b[p++])<<(i*8);}else if(f.wire==2){uint64_t n=ReadVar(b,p);if(n>b.size()-p)throw std::runtime_error("length truncated");f.bytes.assign(b.begin()+p,b.begin()+p+size_t(n));p+=size_t(n);}else throw std::runtime_error("unsupported wire type");out.push_back(std::move(f));}return out;}
inline Bytes Encode(const Message& m){Bytes b;for(const auto& f:m){Var(b,(uint64_t(f.n)<<3)|f.wire);if(f.wire==0)Var(b,f.value);else if(f.wire==1||f.wire==5){for(int i=0;i<(f.wire==1?8:4);++i)b.push_back(uint8_t(f.value>>(i*8)));}else if(f.wire==2){Var(b,f.bytes.size());b.insert(b.end(),f.bytes.begin(),f.bytes.end());}else throw std::runtime_error("wire");if(b.size()>MaxBytes)throw std::runtime_error("size limit");}return b;}
inline Field V(uint32_t n,uint64_t v){Field f;f.n=n;f.value=v;return f;}
inline Field F64(uint32_t n,uint64_t v){Field f=V(n,v);f.wire=1;return f;}
inline Field B(uint32_t n,const Bytes& b){Field f;f.n=n;f.wire=2;f.bytes=b;return f;}
inline uint64_t Get(const Message& m,uint32_t n,uint64_t fallback=0){for(auto it=m.rbegin();it!=m.rend();++it)if(it->n==n&&(it->wire==0||it->wire==1||it->wire==5))return it->value;return fallback;}
inline const Bytes* GetBytes(const Message& m,uint32_t n){for(auto it=m.rbegin();it!=m.rend();++it)if(it->n==n&&it->wire==2)return &it->bytes;return nullptr;}
inline void Remove(Message& m,uint32_t n){m.erase(std::remove_if(m.begin(),m.end(),[n](const Field& f){return f.n==n;}),m.end());}
inline void Set(Message& m,uint32_t n,uint64_t v){Remove(m,n);m.push_back(V(n,v));}
}
constexpr uint32_t ProtoFlag=0x80000000u;
constexpr uint64_t IdPrefix=0xFADA000000000000ULL,IdMask=0xFFFF000000000000ULL;
inline uint64_t FakeId(uint32_t def){return IdPrefix|def;}
inline bool IsFake(uint64_t id){return (id&IdMask)==IdPrefix;}
inline bool HasFake(const pb::Message& m,unsigned depth=0){for(const auto& f:m){if((f.wire==0||f.wire==1)&&IsFake(f.value))return true;if(f.wire==2&&depth<8&&f.bytes.size()<=1024*1024){try{if(HasFake(pb::Decode(f.bytes),depth+1))return true;}catch(...) {}}}return false;}
struct Packet{uint32_t type=0;Bytes data;};
struct Envelope {uint32_t type=0;Bytes header,body;};
inline Envelope Unpack(uint32_t type,const Bytes& b){if(!(type&ProtoFlag)||b.size()<8||b.size()>pb::MaxBytes)throw std::runtime_error("not protobuf envelope");uint32_t t=0,n=0;memcpy(&t,b.data(),4);memcpy(&n,b.data()+4,4);if((t&~ProtoFlag)!=(type&~ProtoFlag)||n>b.size()-8)throw std::runtime_error("bad envelope");Envelope e;e.type=type;e.header.assign(b.begin()+8,b.begin()+8+n);e.body.assign(b.begin()+8+n,b.end());pb::Decode(e.header);return e;}
inline Packet Pack(uint32_t type,const Bytes& body,const Bytes& header={}){Packet p;p.type=type|ProtoFlag;p.data.resize(8);uint32_t n=uint32_t(header.size());memcpy(p.data.data(),&p.type,4);memcpy(p.data.data()+4,&n,4);p.data.insert(p.data.end(),header.begin(),header.end());p.data.insert(p.data.end(),body.begin(),body.end());if(p.data.size()>pb::MaxBytes)throw std::runtime_error("packet limit");return p;}
using Slot=std::pair<uint32_t,uint32_t>;
struct Choice{uint64_t id=0;uint32_t style=0;};
class Core {
 uint64_t owner_=0,version_=0;uint32_t service_=0;
 bool enabled_=false,ready_=false;
 const Definition* defs_;size_t count_;
 std::map<uint64_t,Bytes> real_;
 std::map<Slot,Choice> choices_;
 std::map<uint64_t,uint32_t> itemStyles_;
 std::set<uint32_t> projected_;
 std::deque<Packet> queue_;
 std::map<uint32_t,const Definition*> unique_;
 Bytes Owner()const{return pb::Encode({pb::V(1,1),pb::V(2,owner_)});}
 bool Own(const pb::Message& m,uint32_t field)const{const auto* b=pb::GetBytes(m,field);if(!b)return false;auto o=pb::Decode(*b);return pb::Get(o,1)==1&&pb::Get(o,2)==owner_;}
 const Definition* Find(uint32_t id,uint32_t cls,uint32_t slot)const{for(size_t i=0;i<count_;++i)if(defs_[i].id==id&&defs_[i].cls==cls&&defs_[i].slot==slot)return &defs_[i];return nullptr;}
 static bool StyleAllowed(const Definition& d,uint32_t style){std::string s=","+std::string(d.styles)+",";return s.find(","+std::to_string(style)+",")!=std::string::npos;}
 std::set<uint32_t> Missing()const{std::set<uint32_t> owned;for(const auto& e:real_)owned.insert(uint32_t(pb::Get(pb::Decode(e.second),4)));std::set<uint32_t> missing;for(const auto& e:unique_)if(!owned.count(e.first))missing.insert(e.first);return missing;}
 uint32_t CurrentStyle(uint64_t id)const{auto style=itemStyles_.find(id);if(style!=itemStyles_.end())return style->second;auto it=real_.find(id);return it==real_.end()?0:uint32_t(pb::Get(pb::Decode(it->second),15));}
 uint64_t ChoiceId(uint32_t def)const{for(const auto& e:real_)if(pb::Get(pb::Decode(e.second),4)==def)return e.first;return FakeId(def);}
 Bytes Item(const Definition& d)const{auto m=pb::Message{pb::V(1,FakeId(d.id)),pb::V(2,uint32_t(owner_)),pb::V(3,0),pb::V(4,d.id),pb::V(5,1),pb::V(6,1),pb::V(7,d.quality),pb::V(8,0),pb::V(9,0),pb::V(15,CurrentStyle(FakeId(d.id)))};for(const auto& e:choices_)if(e.second.id==FakeId(d.id)){m.push_back(pb::V(15,e.second.style));m.push_back(pb::B(18,pb::Encode({pb::V(1,e.first.first),pb::V(2,e.first.second)})));}return pb::Encode(m);}
 Bytes Project(const Bytes& raw)const{auto m=pb::Decode(raw);uint64_t id=pb::Get(m,1);if(IsFake(id))return raw;if(itemStyles_.count(id))pb::Set(m,15,itemStyles_.at(id));std::vector<pb::Field> keep;for(const auto& f:m){if(f.n==18&&f.wire==2){auto e=pb::Decode(f.bytes);Slot slot{uint32_t(pb::Get(e,1)),uint32_t(pb::Get(e,2))};auto c=choices_.find(slot);if(c!=choices_.end())continue;}keep.push_back(f);}for(const auto& e:choices_)if(e.second.id==id){pb::Remove(keep,15);keep.push_back(pb::V(15,e.second.style));keep.push_back(pb::B(18,pb::Encode({pb::V(1,e.first.first),pb::V(2,e.first.second)})));}return pb::Encode(keep);}
 void Push(Packet p){
 if((p.type&~ProtoFlag)==26&&!queue_.empty()&&(queue_.back().type&~ProtoFlag)==26){
  auto a=pb::Decode(Unpack(p.type,p.data).body),b=pb::Decode(Unpack(queue_.back().type,queue_.back().data).body);
  auto ordinary=[](const pb::Message& m){return std::none_of(m.begin(),m.end(),[](const pb::Field& f){return f.n==4||f.n==5;});};
  if(ordinary(a)&&ordinary(b)){queue_.back()=std::move(p);return;}
 }
 if(queue_.size()>=32)throw std::runtime_error("local queue full");queue_.push_back(std::move(p));}
 pb::Field Object(uint32_t field,const Bytes& b)const{return pb::B(field,pb::Encode({pb::V(1,1),pb::B(2,b)}));}
 void Delta(bool added,bool removed){
  (void)added;if(!ready_)return;auto missing=removed?std::set<uint32_t>{}:Missing();
  pb::Message msg{pb::F64(3,version_),pb::B(6,Owner())};if(service_)msg.push_back(pb::V(7,service_));
  for(uint32_t def:projected_)if(!missing.count(def))msg.push_back(Object(5,pb::Encode({pb::V(1,FakeId(def))})));
  for(uint32_t def:missing)msg.push_back(Object(projected_.count(def)?2:4,Item(*unique_.at(def))));
  for(const auto& kv:real_)msg.push_back(Object(2,enabled_?Project(kv.second):kv.second));
  Push(Pack(26,pb::Encode(msg)));projected_=std::move(missing);
 }
 bool Cache(pb::Message& cache){if(!Own(cache,4))return false;bool found=false;std::map<uint64_t,Bytes> items;for(const auto& f:cache)if(f.n==2&&f.wire==2){auto type=pb::Decode(f.bytes);if(pb::Get(type,1)!=1)continue;found=true;for(const auto& obj:type)if(obj.n==2&&obj.wire==2){auto item=pb::Decode(obj.bytes);uint64_t id=pb::Get(item,1);if(id&&!IsFake(id)){if(pb::Get(item,2,uint32_t(owner_))!=uint32_t(owner_))throw std::runtime_error("wrong account");items[id]=obj.bytes;}}}
 if(!found)return false;real_=std::move(items);version_=pb::Get(cache,3);service_=uint32_t(pb::Get(cache,5));ready_=true;if(!enabled_)return false;
 for(auto& f:cache)if(f.n==2&&f.wire==2){auto type=pb::Decode(f.bytes);if(pb::Get(type,1)!=1)continue;pb::Remove(type,2);for(const auto& kv:real_)type.push_back(pb::B(2,Project(kv.second)));auto missing=Missing();for(uint32_t def:missing)type.push_back(pb::B(2,Item(*unique_.at(def))));
 pb::Message removed{pb::F64(3,version_),pb::B(6,Owner())};if(service_)removed.push_back(pb::V(7,service_));bool any=false;
 for(uint32_t def:projected_)if(!missing.count(def)){removed.push_back(Object(5,pb::Encode({pb::V(1,FakeId(def))})));any=true;}
 f.bytes=pb::Encode(type);if(any)Push(Pack(26,pb::Encode(removed)));projected_=std::move(missing);return true;}return false;}
 void Reply(uint32_t type,const Bytes& requestHeader,const pb::Message& body){auto h=pb::Decode(requestHeader);pb::Remove(h,11);uint64_t src=pb::Get(h,10,UINT64_MAX);pb::Remove(h,10);pb::Remove(h,12);if(src!=UINT64_MAX)h.push_back(pb::F64(11,src));Push(Pack(type,pb::Encode(body),pb::Encode(h)));}
 bool ValidateChoice(uint64_t id,uint32_t cls,uint32_t slot,uint32_t style)const{if(cls>1000||slot>31)return false;if(!id)return true;if(IsFake(id)){const auto* d=Find(uint32_t(id),cls,slot);return projected_.count(uint32_t(id))&&d&&StyleAllowed(*d,style);}auto it=real_.find(id);if(it==real_.end())return false;auto m=pb::Decode(it->second);const auto* d=Find(uint32_t(pb::Get(m,4)),cls,slot);return d&&StyleAllowed(*d,style);}
 public:
 Core(const Definition* d,size_t n):defs_(d),count_(n){for(size_t i=0;i<n;++i)unique_.emplace(d[i].id,&d[i]);}
 void SetOwner(uint64_t owner){if(owner_==owner)return;owner_=owner;enabled_=false;ready_=false;real_.clear();choices_.clear();itemStyles_.clear();projected_.clear();queue_.clear();version_=0;service_=0;}
 uint64_t OwnerId()const{return owner_;}bool Ready()const{return ready_;}bool Enabled()const{return enabled_;}size_t Count()const{return unique_.size();}size_t Selected()const{return choices_.size();}const std::map<Slot,Choice>& Choices()const{return choices_;}
 void RestoreSelections(const std::map<Slot,std::pair<uint32_t,uint32_t>>& values){auto old=choices_;for(const auto& e:values){uint64_t id=ChoiceId(e.second.first);if(ValidateChoice(id,e.first.first,e.first.second,e.second.second))choices_[e.first]={id,e.second.second};}try{if(enabled_&&ready_)Delta(false,false);}catch(...){choices_=std::move(old);throw;}}
 std::map<Slot,std::pair<uint32_t,uint32_t>> SelectedDefinitions()const{std::map<Slot,std::pair<uint32_t,uint32_t>> out;for(const auto& e:choices_){uint32_t def=IsFake(e.second.id)?uint32_t(e.second.id):0;if(!def){auto real=real_.find(e.second.id);if(real!=real_.end())def=uint32_t(pb::Get(pb::Decode(real->second),4));}if(def)out[e.first]={def,e.second.style};}return out;}
 void Enable(bool on){if(enabled_==on)return;bool old=enabled_;enabled_=on;try{Delta(on,!on);}catch(...){enabled_=old;throw;}}
 bool Select(uint32_t def,uint32_t cls,uint32_t slot,uint32_t style){uint64_t id=ChoiceId(def);if(!enabled_||!ready_||!ValidateChoice(id,cls,slot,style))return false;auto old=choices_;auto oldStyles=itemStyles_;choices_[{cls,slot}]={id,style};itemStyles_[id]=style;try{Delta(false,false);}catch(...){choices_=std::move(old);itemStyles_=std::move(oldStyles);throw;}return true;}
 bool Reset(){auto old=choices_;auto oldStyles=itemStyles_;choices_.clear();itemStyles_.clear();try{if(enabled_)Delta(false,false);}catch(...){choices_=std::move(old);itemStyles_=std::move(oldStyles);throw;}return true;}
 // Return -1 to forward untouched, 0 to consume locally, 4 for invalid/unsupported.
 int Outgoing(uint32_t type,const Bytes& raw){const auto t=type&~ProtoFlag;if(!(type&ProtoFlag)){// Prevent accidental raw-wire fake IDs from leaving the process as well.
  for(size_t i=0;i+8<=raw.size();++i){uint64_t id;memcpy(&id,raw.data()+i,8);if(IsFake(id))return 4;}return -1;}
  try{auto e=Unpack(type,raw);auto m=pb::Decode(e.body);if(!enabled_)return HasFake(m)?4:-1;
   if(t!=1059&&t!=2569&&t!=2577)return HasFake(m)?4:-1;
   if(!ready_||queue_.size()>29)return 4;auto old=choices_;auto previousStyles=itemStyles_;
   if(t==2577){
    uint64_t id=pb::Get(m,1);uint32_t style=uint32_t(pb::Get(m,2,255));
    bool match=false;auto oldStyles=itemStyles_;
    if(IsFake(id)){auto def=unique_.find(uint32_t(id));if(def==unique_.end()||!projected_.count(uint32_t(id))||!StyleAllowed(*def->second,style))return 4;itemStyles_[id]=style;match=true;}else{auto real=real_.find(id);if(real!=real_.end()){uint32_t defId=uint32_t(pb::Get(pb::Decode(real->second),4));auto def=unique_.find(defId);if(def!=unique_.end()&&StyleAllowed(*def->second,style)){itemStyles_[id]=style;match=true;}}}
    for(const auto& c:choices_)if(c.second.id==id){if(!ValidateChoice(id,c.first.first,c.first.second,style)){itemStyles_=std::move(oldStyles);return 4;}match=true;}
    if(!match)return 4;for(auto& c:choices_)if(c.second.id==id)c.second.style=style;
    try{Delta(false,false);Reply(2578,e.header,{pb::V(1,0)});}catch(...){choices_=std::move(old);itemStyles_=std::move(oldStyles);throw;}return 0;
   }
   std::vector<pb::Message> equips;if(t==1059)equips.push_back(m);else for(const auto& f:m)if(f.n==1&&f.wire==2)equips.push_back(pb::Decode(f.bytes));
   if(equips.empty()||equips.size()>128)return 4;
   for(const auto& a:equips){uint64_t id=pb::Get(a,1);uint32_t cls=uint32_t(pb::Get(a,2)),slot=uint32_t(pb::Get(a,3)),style=uint32_t(pb::Get(a,4,255));if(style==255)style=CurrentStyle(id);if(!ValidateChoice(id,cls,slot,style))return 4;}
   for(const auto& a:equips){Slot slot{uint32_t(pb::Get(a,2)),uint32_t(pb::Get(a,3))};uint64_t id=pb::Get(a,1);uint32_t style=uint32_t(pb::Get(a,4,255));if(style==255)style=CurrentStyle(id);if(id){choices_[slot]={id,style};itemStyles_[id]=style;}else choices_.erase(slot);}
   try{Delta(false,false);if(t==2569)Reply(2570,e.header,{pb::F64(1,version_)});}catch(...){choices_=std::move(old);itemStyles_=std::move(previousStyles);throw;}return 0;
  }catch(...){return 4; /* Fail closed for malformed protobuf. */}}
 Packet Incoming(uint32_t type,const Bytes& raw){Packet original{type,raw};if(!owner_)return original;try{auto e=Unpack(type,raw);auto m=pb::Decode(e.body);uint32_t t=type&~ProtoFlag;bool changed=false;
 if(t==24)changed=Cache(m);
 else if(t==4004){for(auto& f:m)if(f.n==3&&f.wire==2){auto cache=pb::Decode(f.bytes);if(Cache(cache)){f.bytes=pb::Encode(cache);changed=true;}}}
 else if((t==21||t==22||t==23)&&Own(m,5)&&pb::Get(m,2)==1){const auto* b=pb::GetBytes(m,3);if(b){auto item=pb::Decode(*b);uint64_t id=pb::Get(item,1);if(id&&!IsFake(id)){if(t==23){real_.erase(id);for(auto it=choices_.begin();it!=choices_.end();)if(it->second.id==id)it=choices_.erase(it);else ++it;}else{real_[id]=*b;if(enabled_){for(auto& f:m)if(f.n==3&&f.wire==2)f.bytes=Project(f.bytes);changed=true;}}version_=pb::Get(m,4,version_);}}}
 else if(t==26&&Own(m,6)){version_=pb::Get(m,3,version_);for(auto& f:m)if((f.n==2||f.n==4||f.n==5)&&f.wire==2){auto o=pb::Decode(f.bytes);if(pb::Get(o,1)!=1)continue;const auto* b=pb::GetBytes(o,2);if(!b)continue;auto item=pb::Decode(*b);uint64_t id=pb::Get(item,1);if(!id||IsFake(id))continue;if(f.n==5){real_.erase(id);for(auto it=choices_.begin();it!=choices_.end();)if(it->second.id==id)it=choices_.erase(it);else ++it;}else{real_[id]=*b;if(enabled_){for(auto& v:o)if(v.n==2&&v.wire==2)v.bytes=Project(v.bytes);f.bytes=pb::Encode(o);changed=true;}}}}
 if(enabled_&&ready_&&t!=24&&t!=4004&&Missing()!=projected_)Delta(false,false);
 if(changed)return Pack(t,pb::Encode(m),e.header);return original;}catch(...){return original;}}
 bool HasQueued()const{return !queue_.empty();}uint32_t QueuedSize()const{return queue_.empty()?0:uint32_t(queue_.front().data.size());}
 bool Pop(Packet& out){if(queue_.empty())return false;out=std::move(queue_.front());queue_.pop_front();return true;}
 Packet Refresh()const{return Pack(28,pb::Encode({pb::B(2,Owner())}));}
};
}
