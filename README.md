VP-AuditService
=======

#### This document is incomplete, please visit http://opentransactions.org/wiki/index.php?title=Category:Voting_Pools for up-to-date information.

Withdrawals
=======

Each participant in the voting pool will operate three physically distinct servers -

* Transaction Server
* Auditing Server
* Key Server

The auditing server audits the OT receipts in real time of all the other participants, particularly looking 
for transactions indicating that a customer wishes to withdraw bitcoins from the pool. The act of sending an 
OT receipt for BTC back to the issuer should include a method of indicating the blockchain address to 
which the withdrawals should be sent.

When the auditing server detects a valid withdrawal request on its own transaction server it creates the 
appropriate bitcoin transaction and passes it around to the other auditing servers for signing. When it 
detects a valid withdrawal request on another transaction server, it stands by to sign the withdrawal transaction.

Withdrawal requests are processed using unspent outputs in the hot series without regard for which server 
originally accepted a particular deposit and which server’s user is requesting a withdrawal.


Deposits
=======



Bootstrapping
=======

Phases of Development
=======

1) Communications
	
	Audit Servers should be able to communicate to eachother through BitMessage (optional),
	and should also be able to communicate with OT Servers.	

2) Funds
	
	Audit Servers should be able to process test funds, Deposits/Withdrawals. 
	They should be able to communicate the status of these funds to other 
	Audit servers in the pool.

3) Pool Integrity

	The integrity of the pool should be able to be tested by removing Audit 
	servers and OT servers at-will.

4) ...



Altering Pool Members
=======

Altering the pool (adding, removing, or replacing members).



Actions
=======

* Adding Funds to Pool
* Removing Funds from Pool
* Check for Inflation
* Adding Members
* Removing Members
* Replacing Members


Dependencies
=======

* xmlrpc-c (on OSX, compiled with "CXXFLAGS='-std=c++11 -stdlib=libc++' ./configure --enable-libxml2-backend; make -j2")
* libxml2
* boost >= 1.54.0

Known Issues
=======

* If you are getting errors such as "API Error 0021: Unexpected API Failure - need more than 0 values to unpack", 
    that means xmlrpc-c and auditor were not compiled with the same standard library. Also make sure that you remembered to use
    "--enable-libxml2-backend" when running "./configure" on xmlrpc-c.


Definitions
=======

* Funds - 

* Pool - A group of independent [Open-Transactions](http://opentransactions.org/) transaction servers who cooperate to store outputs
	on the Bitcoin blockchain in order to provide full backing for bitcoin-denominated receipts.

* Multisig - A type of bitcoin output script which uses OP_CHECKMULTISIG as described in [BIP11](https://github.com/bitcoin/bips/blob/master/bip-0011.mediawiki).

* Inflation - 


Related Projects
=======

* [ServerCore](https://github.com/monetas/ServerCore)
* [Verifier](https://github.com/monetas/verifier)


References
=======

* [Voting Pools Wiki](http://opentransactions.org/wiki/index.php?title=Category:Voting_Pools)

