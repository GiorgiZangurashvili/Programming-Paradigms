#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <inttypes.h>

#include "teller.h"
#include "account.h"
#include "error.h"
#include "debug.h"

/*
 * deposit money into an account
 */
int
Teller_DoDeposit(Bank *bank, AccountNumber accountNum, AccountAmount amount)
{
  assert(amount >= 0);

  DPRINTF('t', ("Teller_DoDeposit(account 0x%"PRIx64" amount %"PRId64")\n",
                accountNum, amount));

  Account *account = Account_LookupByNumber(bank, accountNum);

  sem_wait(&(account->locker));  
  sem_wait(&(bank->branches[AccountNum_GetBranchID(accountNum)].branch_locker));

  if (account == NULL) {
    sem_post (&(account->locker));
    sem_post(&(bank->branches[AccountNum_GetBranchID(accountNum)].branch_locker));
    return ERROR_ACCOUNT_NOT_FOUND;
  }

  Account_Adjust(bank,account, amount, 1);
  sem_post (&(account->locker));
  sem_post(&(bank->branches[AccountNum_GetBranchID(accountNum)].branch_locker));
  return ERROR_SUCCESS;
}

/*
 * withdraw money from an account
 */
int
Teller_DoWithdraw(Bank *bank, AccountNumber accountNum, AccountAmount amount)
{
  assert(amount >= 0);

  DPRINTF('t', ("Teller_DoWithdraw(account 0x%"PRIx64" amount %"PRId64")\n",
                accountNum, amount));

  Account *account = Account_LookupByNumber(bank, accountNum);
  
  sem_wait(&(account->locker));  
  sem_wait(&(bank->branches[AccountNum_GetBranchID(accountNum)].branch_locker));

  if (account == NULL) {
    sem_post (&(account->locker));
    sem_post(&(bank->branches[AccountNum_GetBranchID(accountNum)].branch_locker));
    return ERROR_ACCOUNT_NOT_FOUND;
  }

  if (amount > Account_Balance(account)) {
    sem_post (&(account->locker));
    sem_post(&(bank->branches[AccountNum_GetBranchID(accountNum)].branch_locker));
    return ERROR_INSUFFICIENT_FUNDS;
  }

  Account_Adjust(bank,account, -amount, 1);

  sem_post (&(account->locker));
  sem_post(&(bank->branches[AccountNum_GetBranchID(accountNum)].branch_locker));
  return ERROR_SUCCESS;
}

/*
 * do a tranfer from one account to another account
 */
int
Teller_DoTransfer(Bank *bank, AccountNumber srcAccountNum,
                  AccountNumber dstAccountNum,
                  AccountAmount amount)
{
  assert(amount >= 0);

  if(srcAccountNum == dstAccountNum){
    return ERROR_SUCCESS;
  }

  DPRINTF('t', ("Teller_DoTransfer(src 0x%"PRIx64", dst 0x%"PRIx64
                ", amount %"PRId64")\n",
                srcAccountNum, dstAccountNum, amount));

  Account *srcAccount = Account_LookupByNumber(bank, srcAccountNum);
  if (srcAccount == NULL) {
    return ERROR_ACCOUNT_NOT_FOUND;
  }

  Account *dstAccount = Account_LookupByNumber(bank, dstAccountNum);

  if (dstAccount == NULL) {
    return ERROR_ACCOUNT_NOT_FOUND;
  }
  

  /*
   * If we are doing a transfer within the branch, we tell the Account module to
   * not bother updating the branch balance since the net change for the
   * branch is 0.
   */
  int updateBranch = !Account_IsSameBranch(srcAccountNum, dstAccountNum);

  if(!updateBranch){
    if(srcAccount->accountNumber < dstAccount->accountNumber) {   
      sem_wait(&(srcAccount->locker));                        
      sem_wait(&(dstAccount->locker));
    } else {
      sem_wait(&(dstAccount->locker));                          
      sem_wait(&(srcAccount->locker));
    }
    if(amount > Account_Balance(srcAccount)){
      sem_post(&(srcAccount->locker));
      sem_post(&(dstAccount->locker));
      return ERROR_INSUFFICIENT_FUNDS;
    }else{
      Account_Adjust(bank, srcAccount, -amount, updateBranch);
      Account_Adjust(bank, dstAccount, amount, updateBranch);
      sem_post(&(srcAccount->locker));
      sem_post(&(dstAccount->locker));
      return ERROR_SUCCESS;
    }
  }else{
      int src_id = AccountNum_GetBranchID(srcAccountNum);
      int dst_id = AccountNum_GetBranchID(dstAccountNum);
      if (src_id < dst_id) {
      sem_wait(&(srcAccount->locker));            
      sem_wait(&(dstAccount->locker));         
      sem_wait(&(bank->branches[src_id].branch_locker));   
      sem_wait(&(bank->branches[dst_id].branch_locker));
    } else {
      sem_wait(&(dstAccount->locker));
      sem_wait(&(srcAccount->locker));
      sem_wait(&(bank->branches[dst_id].branch_locker));
      sem_wait(&(bank->branches[src_id].branch_locker));
    }
    if(amount > Account_Balance(srcAccount)){
      sem_post(&(srcAccount->locker));
      sem_post(&(dstAccount->locker));
      sem_post(&(bank->branches[dst_id].branch_locker));
      sem_post(&(bank->branches[src_id].branch_locker));
      return ERROR_INSUFFICIENT_FUNDS;
    }else{
      Account_Adjust(bank, srcAccount, -amount, updateBranch);
      Account_Adjust(bank, dstAccount, amount, updateBranch);
      sem_post(&(srcAccount->locker));
      sem_post(&(dstAccount->locker));
      sem_post(&(bank->branches[dst_id].branch_locker));
      sem_post(&(bank->branches[src_id].branch_locker));
      return ERROR_SUCCESS;
    }
  }
  return ERROR_SUCCESS;
}
