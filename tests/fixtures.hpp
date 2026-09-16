#pragma once
#include "aquarium/domain.hpp"

namespace aq::testing {
// A test may supply a wallet or account level directly. Record that state as
// its opening balance so subsequent real commands still reconcile on reload.
inline void openingBalances(State& s){
 s.lifetimeXp=s.xp;s.ledger=Json::array();s.receipts=Json::object();s.settlements=Json::object();s.revision=0;
 for(const auto& [currency,amount]:std::vector<std::pair<std::string,Amount>>{{"coins",s.wallet.coins},{"pearls",s.wallet.pearls},{"xp",s.xp}})if(amount)
  s.ledger.push_back({{"id",s.ledger.size()+1},{"currency",currency},{"amount",amount},{"balance_after",amount},{"reason","fixture"},{"source_id",currency},{"request_id","fixture"},{"created_at",s.calendarNow},{"revision",0}});
}
inline void stage(Fish& f,int age){f.egg=false;f.age=age;f.growthMs=f.purchase.durationMs/10000*f.purchase.stages[age];}
}
