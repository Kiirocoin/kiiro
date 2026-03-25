# 🚨 Kiirocoin Masternode Collateral Update! 🚨

**Effective Block:** **600,000**  
**New Required Collateral:** **40,000 KIIRO**  
_All masternode owners must upgrade and update collateral before block 600,000 or your masternode will stop receiving payments._

...

# 🚨 Kiirocoin Masternode Collateral Update! 🚨

Effective Block: 600,000  
New Required Collateral: 40,000 KIIRO  
_All masternode owners must upgrade and update collateral before block 600,000 or your masternode will stop receiving payments._

---

## 🏃 Step-by-Step Masternode Update Guide

### Step 1: Upgrade Your Wallet/Daemon

1. Download the latest release (v1.0.0.7) for your system at:  
   [https://github.com/Kiirocoin/kiiro/releases](https://github.com/Kiirocoin/kiiro/releases)
2. Stop your old Kiirocoin daemon:  
   
   kiirocoin-cli stop
   
3. Replace the old binaries with the new ones (kiirocoind, kiirocoin-cli, etc.).
4. Start the new daemon:  
   
   kiirocoind -daemon
   

---

### Step 2: Prepare 40,000 KIIRO Collateral

1. Send exactly 40,000 KIIRO to a new wallet address (do NOT reuse old masternode addresses):  
   
   kiirocoin-cli getnewaddress
   
2. Record the collateral TXID and output index after your transaction is confirmed:  
   
   kiirocoin-cli listunspent
   
   Look for the txid and vout matching 40,000 KIIRO.

---

### Step 3: Generate a BLS key for your masternode (if starting fresh)

On the controller wallet:
kiirocoin-cli bls generate
Copy and save both keys, especially secret.

---

### Step 4: Create/update your masternode registration

If new masternode / moving collateral  
> Use the `protx register_fund` command:
kiirocoin-cli protx register_fund <collateralAddress> <ip:port> <ownerKeyAddr> <operatorPubKey_or_bls> <votingKeyAddr> <operatorReward> <payoutAddress>
- Replace the fields with your data. See example below.

If upgrading existing masternode to 40,000 collateral (after payout/block 600k),  
> Use `protx update_reg`:  
(you may need to re-register/submit new protx with new collateral).

#### Example: Registering a New Masternode
kiirocoin-cli protx register_fund \
   "YOUR_COLLATERAL_ADDRESS" \
   "YOUR_SERVER_IP:MN_PORT" \
   "OWNER_KEY_ADDRESS" \
   "BLS_PUBLIC_KEY" \
   "VOTING_KEY_ADDRESS" \
   0 \
   "PAYOUT_ADDRESS"
- All addresses must be in quotes!  
- The command will return a ProTx hash. Save this for masternode.conf.

---

### Step 5: Update your masternode.conf

1. On the controller wallet, update your masternode.conf:
   
   <alias> <txid> <vout> <masternode_private_key> <IP:port>
   
2. Restart your masternode hot/vps wallet using the BLS/private key pairs.

---

### Step 6: Start Your Masternode

From controller wallet (after confirmations):
kiirocoin-cli protx list
Check status, payout address, and updated collateral.

---

## ❗️ Important Notes

- Backup your wallet.dat and keys before any changes.
- Only masternodes with 40,000 KIIRO collateral will be paid after block 600,000.
- Do not reuse old addresses for your new collateral.
- Check required confirmations (usually 15+) before registering new collateral.

---

## 📖 Quick Reference: Useful CLI Commands

- Get a new address:  
  
  kiirocoin-cli getnewaddress
  
- Check your funds and TXID:  
  
  kiirocoin-cli listunspent
  
- Generate BLS keys (for deterministic masternodes):  
  
  kiirocoin-cli bls generate
  
- Register a protx (hot->cold):  
  
  kiirocoin-cli protx register_fund <...>
  
- Check masternode status:  
  
  kiirocoin-cli protx list
  kiirocoin-cli getmasternodelist full
  

---

Please upgrade early to avoid missing MN payments or losing your place in the payment queue!
