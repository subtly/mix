/*
	This file is part of cpp-ethereum.

	cpp-ethereum is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	cpp-ethereum is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with cpp-ethereum.  If not, see <http://www.gnu.org/licenses/>.
*/
/** @file ClientModel.h
 * @author Yann yann@ethdev.com
 * @author Arkadiy Paronyan arkadiy@ethdev.com
 * @date 2015
 * Ethereum IDE client.
 */

#pragma once

#include <atomic>
#include <map>
#include <QString>
#include <QVariantMap>
#include "QBigInt.h"
#include "MachineStates.h"

namespace dev
{
namespace mix
{

class AppContext;
class Web3Server;
class RpcConnector;
class QEther;
class QDebugData;
class MixClient;
class QVariableDefinition;

/// Backend transaction config class
struct TransactionSettings
{
	TransactionSettings() {}
	TransactionSettings(QString const& _contractId, QString const& _functionId, u256 _value, u256 _gas, u256 _gasPrice, Secret _sender):
		contractId(_contractId), functionId(_functionId), value(_value), gas(_gas), gasPrice(_gasPrice), sender(_sender) {}
	TransactionSettings(QString const& _stdContractName, QString const& _stdContractUrl):
		contractId(_stdContractName), stdContractUrl(_stdContractUrl) {}

	/// Contract name
	QString contractId;
	/// Contract function name
	QString functionId;
	/// Transaction value
	u256 value;
	/// Gas
	u256 gas;
	/// Gas price
	u256 gasPrice;
	/// Mapping from contract function parameter name to value
	QList<QVariableDefinition*> parameterValues;
	/// Standard contract url
	QString stdContractUrl;
	/// Sender
	Secret sender;
};


/// UI Transaction log record
class RecordLogEntry: public QObject
{
	Q_OBJECT
	Q_ENUMS(RecordType)
	/// Recording index
	Q_PROPERTY(unsigned recordIndex MEMBER m_recordIndex CONSTANT)
	/// Human readable transaction bloack and transaction index
	Q_PROPERTY(QString transactionIndex MEMBER m_transactionIndex CONSTANT)
	/// Contract name if any
	Q_PROPERTY(QString contract MEMBER m_contract CONSTANT)
	/// Function name if any
	Q_PROPERTY(QString function MEMBER m_function CONSTANT)
	/// Transaction value
	Q_PROPERTY(QString value MEMBER m_value CONSTANT)
	/// Receiving address
	Q_PROPERTY(QString address MEMBER m_address CONSTANT)
	/// Returned value or transaction address in case of creation
	Q_PROPERTY(QString returned MEMBER m_returned CONSTANT)
	/// true if call, false if transaction
	Q_PROPERTY(bool call MEMBER m_call CONSTANT)
	/// @returns record type
	Q_PROPERTY(RecordType type MEMBER m_type CONSTANT)

public:
	enum RecordType
	{
		Transaction,
		Block
	};

	RecordLogEntry():
		m_recordIndex(0), m_call(false), m_type(RecordType::Transaction) {}
	RecordLogEntry(unsigned _recordIndex, QString _transactionIndex, QString _contract, QString _function, QString _value, QString _address, QString _returned, bool _call, RecordType _type):
		m_recordIndex(_recordIndex), m_transactionIndex(_transactionIndex), m_contract(_contract), m_function(_function), m_value(_value), m_address(_address), m_returned(_returned), m_call(_call), m_type(_type) {}

private:
	unsigned m_recordIndex;
	QString m_transactionIndex;
	QString m_contract;
	QString m_function;
	QString m_value;
	QString m_address;
	QString m_returned;
	bool m_call;
	RecordType m_type;
};

/**
 * @brief Ethereum state control
 */
class ClientModel: public QObject
{
	Q_OBJECT

public:
	ClientModel(AppContext* _context);
	~ClientModel();
	/// @returns true if currently executing contract code
	Q_PROPERTY(bool running MEMBER m_running NOTIFY runStateChanged)
	/// @returns true if currently mining
	Q_PROPERTY(bool mining MEMBER m_mining NOTIFY miningStateChanged)
	/// @returns deployed contracts addresses
	Q_PROPERTY(QVariantMap contractAddresses READ contractAddresses NOTIFY contractAddressesChanged)
	// @returns the last block
	Q_PROPERTY(RecordLogEntry* lastBlock READ lastBlock CONSTANT)
	/// ethereum.js RPC request entry point
	/// @param _message RPC request in Json format
	/// @returns RPC response in Json format
	Q_INVOKABLE QString apiCall(QString const& _message);
	/// Simulate mining. Creates a new block
	Q_INVOKABLE void mine();

public slots:
	/// Setup state, run transaction sequence, show debugger for the last transaction
	/// @param _state JS object with state configuration
	void setupState(QVariantMap _state);
	/// Show the debugger for a specified record
	Q_INVOKABLE void debugRecord(unsigned _index);
	/// Show the debugger for an empty record
	Q_INVOKABLE void emptyRecord();
	Q_INVOKABLE QString newAddress();

private slots:
	/// Update UI with machine states result. Display a modal dialog.
	void showDebugger();
	/// Update UI with transaction run error.
	void showDebugError(QString const& _error);

signals:
	/// Transaction execution started
	void runStarted();
	/// Transaction execution completed successfully
	void runComplete();
	/// Mining has started
	void miningStarted();
	/// Mined a new block
	void miningComplete();
	/// Mining stopped or started
	void miningStateChanged();
	/// Transaction execution completed with error
	/// @param _message Error message
	void runFailed(QString const& _message);
	/// Contract address changed
	void contractAddressesChanged();
	/// Execution state changed
	void newBlock();
	/// Execution state changed
	void runStateChanged();
	/// Show debugger window request
	void debugDataReady(QObject* _debugData);
	/// ethereum.js RPC response ready
	/// @param _message RPC response in Json format
	void apiResponse(QString const& _message);
	/// New transaction log entry
	void newRecord(RecordLogEntry* _r);
	/// State (transaction log) cleared
	void stateCleared();

private:
	RecordLogEntry* lastBlock() const;
	QVariantMap contractAddresses() const;
	void executeSequence(std::vector<TransactionSettings> const& _sequence, std::map<Secret, u256> const& _balances);
	dev::Address deployContract(bytes const& _code, TransactionSettings const& _tr = TransactionSettings());
	void callContract(Address const& _contract, bytes const& _data, TransactionSettings const& _tr);
	void onNewTransaction();
	void onStateReset();
	void showDebuggerForTransaction(ExecutionResult const& _t);

	AppContext* m_context;
	std::atomic<bool> m_running;
	std::atomic<bool> m_mining;
	std::unique_ptr<MixClient> m_client;
	std::unique_ptr<RpcConnector> m_rpcConnector;
	std::unique_ptr<Web3Server> m_web3Server;
	std::map<QString, Address> m_contractAddresses;
	std::map<Address, QString> m_contractNames;
	std::map<QString, Address> m_stdContractAddresses;
	std::map<Address, QString> m_stdContractNames;
};

}
}

Q_DECLARE_METATYPE(dev::mix::RecordLogEntry*)
