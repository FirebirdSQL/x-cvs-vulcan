#include <stdio.h>
#include "firebird.h"
#include "common.h"
#include "InternalDatabaseMetaData.h"
#include "InternalConnection.h"
#include "Attachment.h"
#include "SQLError.h"
#include "InternalResultSet.h"
#include "InternalTablesResultSet.h"
#include "InternalColumnsResultSet.h"
#include "InternalIndexInfoResultSet.h"
#include "InternalPrimaryKeysResultSet.h"
#include "InternalCrossReferenceResultSet.h"
#include "InternalProceduresResultSet.h"
#include "InternalProcedureColumnsResultSet.h"

#define DRIVER_VERSION	"T1.0A"
#define MAJOR_VERSION	1
#define MINOR_VERSION	0

#define TRANSACTION_READ_UNCOMMITTED  1

/**
 * Dirty reads are prevented; non-repeatable reads and phantom
 * reads can occur.
 */
#define TRANSACTION_READ_COMMITTED    2

/**
 * Dirty reads and non-repeatable reads are prevented; phantom
 * reads can occur.     
 */
#define TRANSACTION_REPEATABLE_READ   4

/**
 * Dirty reads, non-repeatable reads and phantom reads are prevented.
 */
#define TRANSACTION_SERIALIZABLE      8


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

InternalDatabaseMetaData::InternalDatabaseMetaData(InternalConnection *connect)
{
	connection = connect;
}

InternalDatabaseMetaData::~InternalDatabaseMetaData()
{

}

ResultSet* InternalDatabaseMetaData::getTables(const char * catalog, const char * schemaPattern, const char * tableNamePattern, int typeCount, const char **types)
{
	InternalTablesResultSet *resultSet = new InternalTablesResultSet (this);

	try
		{
		resultSet->getTables (catalog, schemaPattern, tableNamePattern, typeCount, types);
		}
	catch (...)
		{
		delete resultSet;
		throw;
		}

	return resultSet;
}

ResultSet* InternalDatabaseMetaData::getColumns(const char * catalog, const char * schemaPattern, const char * tableNamePattern, const char * fieldNamePattern)
{
	InternalColumnsResultSet *resultSet = new InternalColumnsResultSet (this);

	try
		{
		resultSet->getColumns (catalog, schemaPattern, tableNamePattern, fieldNamePattern);
		}
	catch (...)
		{
		delete resultSet;
		throw;
		}

	return resultSet;
}

ResultSet* InternalDatabaseMetaData::getPrimaryKeys(const char * catalog, const char * schemaPattern, const char * tableNamePattern)
{
	InternalPrimaryKeysResultSet *resultSet = new InternalPrimaryKeysResultSet (this);

	try
		{
		resultSet->getPrimaryKeys (catalog, schemaPattern, tableNamePattern);
		}
	catch (...)
		{
		delete resultSet;
		throw;
		}

	return resultSet;
}

ResultSet* InternalDatabaseMetaData::getImportedKeys(const char * catalog, const char * schemaPattern, const char * tableNamePattern)
{
	NOT_YET_IMPLEMENTED;
	InternalResultSet *resultSet = new InternalResultSet (NULL);

	return (ResultSet*) resultSet;
}

ResultSet* InternalDatabaseMetaData::getIndexInfo(const char * catalog, 
											 const char * schemaPattern, 
											 const char * tableNamePattern, 
											 bool unique, bool approximate)
{
	InternalIndexInfoResultSet *resultSet = new InternalIndexInfoResultSet (this);

	try
		{
		resultSet->getIndexInfo (catalog, schemaPattern, tableNamePattern, unique, approximate);
		}
	catch (...)
		{
		delete resultSet;
		throw;
		}

	return resultSet;
}

ResultSet* InternalDatabaseMetaData::getObjectPrivileges(const char * catalog, const char * schemaPattern, const char * namePattern, int objectType)
{
	NOT_YET_IMPLEMENTED;
	InternalResultSet *resultSet = new InternalResultSet (NULL);

	return (ResultSet*) resultSet;
}

ResultSet* InternalDatabaseMetaData::getUserRoles(const char * user)
{
	NOT_YET_IMPLEMENTED;
	InternalResultSet *resultSet = new InternalResultSet (NULL);

	return (ResultSet*) resultSet;
}

ResultSet* InternalDatabaseMetaData::getRoles(const char * catalog, const char * schemaPattern, const char *rolePattern)
{
	NOT_YET_IMPLEMENTED;
	InternalResultSet *resultSet = new InternalResultSet (NULL);

	return (ResultSet*) resultSet;
}

ResultSet* InternalDatabaseMetaData::getUsers(const char * catalog, const char *userPattern)
{
	NOT_YET_IMPLEMENTED;
	InternalResultSet *resultSet = new InternalResultSet (NULL);

	return (ResultSet*) resultSet;
}

bool InternalDatabaseMetaData::allProceduresAreCallable()
	{
	return true;
	}

bool InternalDatabaseMetaData::allTablesAreSelectable()
	{
	return true;
	}

const char* InternalDatabaseMetaData::getURL()
	{
	NOT_YET_IMPLEMENTED;
	return 0;
	}

const char* InternalDatabaseMetaData::getUserName()
	{
	//return connection->attachment->userName;
	NOT_YET_IMPLEMENTED;
	}

bool InternalDatabaseMetaData::isReadOnly()
	{
	return false;
	}

bool InternalDatabaseMetaData::nullsAreSortedHigh()
	{
	return false;
	}

bool InternalDatabaseMetaData::nullsAreSortedLow()
	{
	return false;
	}

bool InternalDatabaseMetaData::nullsAreSortedAtStart()
	{
	return false;
	}

bool InternalDatabaseMetaData::nullsAreSortedAtEnd()
	{
	return true;
	}

const char* InternalDatabaseMetaData::getDatabaseProductName()
	{
	return "InterBase";
	}

const char* InternalDatabaseMetaData::getDatabaseProductVersion()
	{
	//return connection->attachment->databaseVersion;
	NOT_YET_IMPLEMENTED;
	}

const char* InternalDatabaseMetaData::getDriverName()
	{
	return "InternalDbc";
	}

const char* InternalDatabaseMetaData::getDriverVersion()
	{
	return DRIVER_VERSION;
	}

int InternalDatabaseMetaData::getDriverMajorVersion()
	{
	return MAJOR_VERSION;
	}

int InternalDatabaseMetaData::getDriverMinorVersion()
	{
	return MINOR_VERSION;
	}

bool InternalDatabaseMetaData::usesLocalFiles()
	{
	return false;
	}

bool InternalDatabaseMetaData::usesLocalFilePerTable()
	{
	return false;
	}

bool InternalDatabaseMetaData::supportsMixedCaseIdentifiers()
	{
	return true;
	}

bool InternalDatabaseMetaData::storesUpperCaseIdentifiers()
	{
	return true;
	}

bool InternalDatabaseMetaData::storesLowerCaseIdentifiers()
	{
	return false;
	}

bool InternalDatabaseMetaData::storesMixedCaseIdentifiers()
	{
	return false;
	}

bool InternalDatabaseMetaData::supportsMixedCaseQuotedIdentifiers()
	{
	return true;
	}

bool InternalDatabaseMetaData::storesUpperCaseQuotedIdentifiers()
	{
	return false;
	}

bool InternalDatabaseMetaData::storesLowerCaseQuotedIdentifiers()
	{
	return false;
	}

bool InternalDatabaseMetaData::storesMixedCaseQuotedIdentifiers()
	{
	//return connection->attachment->quotedIdentifiers;
	NOT_YET_IMPLEMENTED;
	}

const char* InternalDatabaseMetaData::getIdentifierQuoteString()
	{
	//return (connection->attachment->quotedIdentifiers) ? "\"" : "";
	NOT_YET_IMPLEMENTED;
	}

const char* InternalDatabaseMetaData::getSQLKeywords()
	{
	return "WEEKDAY,YEARDAY,SQL,TRIGGER";
	}

const char* InternalDatabaseMetaData::getNumericFunctions()
	{
	NOT_YET_IMPLEMENTED;
	return 0;
	}

const char* InternalDatabaseMetaData::getStringFunctions()
	{
	NOT_YET_IMPLEMENTED;
	return 0;
	}

const char* InternalDatabaseMetaData::getSystemFunctions()
	{
	NOT_YET_IMPLEMENTED;
	return 0;
	}

const char* InternalDatabaseMetaData::getTimeDateFunctions()
	{
	NOT_YET_IMPLEMENTED;
	return 0;
	}

const char* InternalDatabaseMetaData::getSearchStringEscape()
	{
	NOT_YET_IMPLEMENTED;
	return 0;
	}

const char* InternalDatabaseMetaData::getExtraNameCharacters()
	{
	NOT_YET_IMPLEMENTED;
	return 0;
	}

bool InternalDatabaseMetaData::supportsAlterTableWithAddColumn()
	{
	return true;
	}

bool InternalDatabaseMetaData::supportsAlterTableWithDropColumn()
	{
	return true;
	}

bool InternalDatabaseMetaData::supportsColumnAliasing()
	{
	return true;
	}

bool InternalDatabaseMetaData::nullPlusNonNullIsNull()
	{
	return true;
	}

bool InternalDatabaseMetaData::supportsConvert()
	{
	return true;
	}

bool InternalDatabaseMetaData::supportsConvert(int fromType, int toType)
	{
	NOT_YET_IMPLEMENTED;
	return 0;
	}

bool InternalDatabaseMetaData::supportsTableCorrelationNames()
	{
	return true;
	}

bool InternalDatabaseMetaData::supportsDifferentTableCorrelationNames()
	{
	return false;
	}

bool InternalDatabaseMetaData::supportsExpressionsInOrderBy()
	{
	return false;
	}

bool InternalDatabaseMetaData::supportsOrderByUnrelated()
	{
	return true;
	}

bool InternalDatabaseMetaData::supportsGroupBy()
	{
	return true;
	}

bool InternalDatabaseMetaData::supportsGroupByUnrelated()
	{
	return true;
	}

bool InternalDatabaseMetaData::supportsGroupByBeyondSelect()
	{
	return true;
	}

bool InternalDatabaseMetaData::supportsLikeEscapeClause()
	{
	return true;
	}

bool InternalDatabaseMetaData::supportsMultipleResultSets()
	{
	return false;
	}

bool InternalDatabaseMetaData::supportsMultipleTransactions()
	{
	return true;
	}

bool InternalDatabaseMetaData::supportsNonNullableColumns()
	{
	return true;
	}

bool InternalDatabaseMetaData::supportsMinimumSQLGrammar()
	{
	return true;
	}

bool InternalDatabaseMetaData::supportsCoreSQLGrammar()
	{
	return true;
	}

bool InternalDatabaseMetaData::supportsExtendedSQLGrammar()
	{
	return false;
	}

bool InternalDatabaseMetaData::supportsANSI92EntryLevelSQL()
	{
	return true;
	}

bool InternalDatabaseMetaData::supportsANSI92IntermediateSQL()
	{
	return false;
	}

bool InternalDatabaseMetaData::supportsANSI92FullSQL()
	{
	return false;
	}

bool InternalDatabaseMetaData::supportsIntegrityEnhancementFacility()
	{
	return false;
	}

bool InternalDatabaseMetaData::supportsOuterJoins()
	{
	return true;
	}

bool InternalDatabaseMetaData::supportsFullOuterJoins()
	{
	return true;
	}

bool InternalDatabaseMetaData::supportsLimitedOuterJoins()
	{
	return true;
	}

const char* InternalDatabaseMetaData::getSchemaTerm()
	{
	return "schema";
	}

const char* InternalDatabaseMetaData::getProcedureTerm()
	{
	return "procedure";
	}

const char* InternalDatabaseMetaData::getCatalogTerm()
	{
	return "system tables";
	}

bool InternalDatabaseMetaData::isCatalogAtStart()
	{
	return true;
	}

const char* InternalDatabaseMetaData::getCatalogSeparator()
	{
	return ".";
	}

bool InternalDatabaseMetaData::supportsSchemasInDataManipulation()
	{
	return false;
	}

bool InternalDatabaseMetaData::supportsSchemasInProcedureCalls()
	{
	return false;
	}

bool InternalDatabaseMetaData::supportsSchemasInTableDefinitions()
	{
	return false;
	}

bool InternalDatabaseMetaData::supportsSchemasInIndexDefinitions()
	{
	return false;
	}

bool InternalDatabaseMetaData::supportsSchemasInPrivilegeDefinitions()
	{
	return false;
	}

bool InternalDatabaseMetaData::supportsCatalogsInDataManipulation()
	{
	return false;
	}

bool InternalDatabaseMetaData::supportsCatalogsInProcedureCalls()
	{
	return false;
	}

bool InternalDatabaseMetaData::supportsCatalogsInTableDefinitions()
	{
	return false;
	}

bool InternalDatabaseMetaData::supportsCatalogsInIndexDefinitions()
	{
	return false;
	}

bool InternalDatabaseMetaData::supportsCatalogsInPrivilegeDefinitions()
	{
	return false;
	}

bool InternalDatabaseMetaData::supportsPositionedDelete()
	{
	return true;
	}

bool InternalDatabaseMetaData::supportsPositionedUpdate()
	{
	return true;
	}

bool InternalDatabaseMetaData::supportsSelectForUpdate()
	{
	return true;
	}

bool InternalDatabaseMetaData::supportsStoredProcedures()
	{
	return true;
	}

bool InternalDatabaseMetaData::supportsSubqueriesInComparisons()
	{
	return true;
	}

bool InternalDatabaseMetaData::supportsSubqueriesInExists()
	{
	return true;
	}

bool InternalDatabaseMetaData::supportsSubqueriesInIns()
	{
	return true;
	}

bool InternalDatabaseMetaData::supportsSubqueriesInQuantifieds()
	{
	return true;
	}

bool InternalDatabaseMetaData::supportsCorrelatedSubqueries()
	{
	return true;
	}

bool InternalDatabaseMetaData::supportsUnion()
	{
	return true;
	}

bool InternalDatabaseMetaData::supportsUnionAll()
	{
	return true;
	}

bool InternalDatabaseMetaData::supportsOpenCursorsAcrossCommit()
	{
	return false;
	}

bool InternalDatabaseMetaData::supportsOpenCursorsAcrossRollback()
	{
	return false;
	}

bool InternalDatabaseMetaData::supportsOpenStatementsAcrossCommit()
	{
	return true;
	}

bool InternalDatabaseMetaData::supportsOpenStatementsAcrossRollback()
	{
	return true;
	}

int InternalDatabaseMetaData::getMaxCharLiteralLength()
	{
	NOT_YET_IMPLEMENTED;
	return 0;
	}

int InternalDatabaseMetaData::getMaxColumnNameLength()
	{
	return 31;
	}

int InternalDatabaseMetaData::getMaxColumnsInGroupBy()
	{
	NOT_YET_IMPLEMENTED;
	return 0;
	}

int InternalDatabaseMetaData::getMaxColumnsInIndex()
	{
	return 16;
	}

int InternalDatabaseMetaData::getMaxColumnsInOrderBy()
	{
	NOT_YET_IMPLEMENTED;
	return 0;
	}

int InternalDatabaseMetaData::getMaxColumnsInSelect()
	{
	NOT_YET_IMPLEMENTED;
	return 0;
	}

int InternalDatabaseMetaData::getMaxColumnsInTable()
	{
	return 32767;
	}

int InternalDatabaseMetaData::getMaxConnections()
	{
	return 32767;
	}

int InternalDatabaseMetaData::getMaxCursorNameLength()
	{
	return 31;
	}

int InternalDatabaseMetaData::getMaxIndexLength()
	{
	return 250;
	}

int InternalDatabaseMetaData::getMaxSchemaNameLength()
	{
	return 0;
	}

int InternalDatabaseMetaData::getMaxProcedureNameLength()
	{
	return 31;
	}

int InternalDatabaseMetaData::getMaxCatalogNameLength()
	{
	return 0;
	}

int InternalDatabaseMetaData::getMaxRowSize()
	{
	return 65535;
	}

bool InternalDatabaseMetaData::doesMaxRowSizeIncludeBlobs()
	{
	return false;
	}

int InternalDatabaseMetaData::getMaxStatementLength()
	{
	NOT_YET_IMPLEMENTED;
	return 0;
	}

int InternalDatabaseMetaData::getMaxStatements()
	{
	return 65535;
	}

int InternalDatabaseMetaData::getMaxTableNameLength()
	{
	return 31;
	}

int InternalDatabaseMetaData::getMaxTablesInSelect()
	{
	return 128;
	}

int InternalDatabaseMetaData::getMaxUserNameLength()
	{
	return 31;
	}

int InternalDatabaseMetaData::getDefaultTransactionIsolation()
	{
	return TRANSACTION_SERIALIZABLE;
	}

bool InternalDatabaseMetaData::supportsTransactions()
	{
	return true;
	}

bool InternalDatabaseMetaData::supportsTransactionIsolationLevel(int level)
	{
	switch (level)
		{
		case TRANSACTION_READ_UNCOMMITTED:
			return false;

		/**
		 * Dirty reads are prevented; non-repeatable reads and phantom
		 * reads can occur.
		 */
		case TRANSACTION_READ_COMMITTED:
			return true;

		/**
		 * Dirty reads and non-repeatable reads are prevented; phantom
		 * reads can occur.     
		 */
		case TRANSACTION_REPEATABLE_READ:
			return true;

		/**
		 * Dirty reads, non-repeatable reads and phantom reads are prevented.
		 */
		case TRANSACTION_SERIALIZABLE:
			return true;
		}

	return false;
	}

bool InternalDatabaseMetaData::supportsDataDefinitionAndDataManipulationTransactions()
	{
	return true;
	}

bool InternalDatabaseMetaData::supportsDataManipulationTransactionsOnly()
	{
	return false;
	}

bool InternalDatabaseMetaData::dataDefinitionCausesTransactionCommit()
	{
	return false;
	}

bool InternalDatabaseMetaData::dataDefinitionIgnoredInTransactions()
	{
	return false;
	}

ResultSet* InternalDatabaseMetaData::getProcedures(const char* catalog, const char* schemaPattern,
		const char* procedureNamePattern)
	{
	InternalProceduresResultSet *resultSet = new InternalProceduresResultSet (this);

	try
		{
		resultSet->getProcedures (catalog, schemaPattern, procedureNamePattern);
		}
	catch (...)
		{
		delete resultSet;
		throw;
		}

	return resultSet;
	}


ResultSet* InternalDatabaseMetaData::getProcedureColumns(const char* catalog,
		const char* schemaPattern,
		const char* procedureNamePattern, 
		const char* columnNamePattern)
	{
	InternalProcedureColumnsResultSet *resultSet = new InternalProcedureColumnsResultSet (this);

	try
		{
		resultSet->getProcedureColumns (catalog, schemaPattern, procedureNamePattern, columnNamePattern);
		}
	catch (...)
		{
		delete resultSet;
		throw;
		}

	return resultSet;
	}


ResultSet* InternalDatabaseMetaData::getSchemas()
	{
	NOT_YET_IMPLEMENTED;
	return 0;
	}

ResultSet* InternalDatabaseMetaData::getCatalogs()
	{
	NOT_YET_IMPLEMENTED;
	return 0;
	}

ResultSet* InternalDatabaseMetaData::getTableTypes()
	{
	NOT_YET_IMPLEMENTED;
	return 0;
	}

ResultSet* InternalDatabaseMetaData::getColumnPrivileges(const char* catalog, const char* schema,
	const char* table, const char* columnNamePattern)
	{
	NOT_YET_IMPLEMENTED;
	return 0;
	}


ResultSet* InternalDatabaseMetaData::getTablePrivileges(const char* catalog, const char* schemaPattern,
			const char* tableNamePattern)
	{
	NOT_YET_IMPLEMENTED;
	return 0;
	}


ResultSet* InternalDatabaseMetaData::getBestRowIdentifier(const char* catalog, const char* schema,
	const char* table, int scope, bool nullable)
	{
	NOT_YET_IMPLEMENTED;
	return 0;
	}


ResultSet* InternalDatabaseMetaData::getVersionColumns(const char* catalog, const char* schema,
			const char* table)
	{
	NOT_YET_IMPLEMENTED;
	return 0;
	}


ResultSet* InternalDatabaseMetaData::getExportedKeys(const char* catalog, const char* schema,
			const char* table)
	{
	NOT_YET_IMPLEMENTED;
	return 0;
	}


ResultSet* InternalDatabaseMetaData::getCrossReference(
		const char* primaryCatalog, const char* primarySchema, const char* primaryTable,
		const char* foreignCatalog, const char* foreignSchema, const char* foreignTable
	)

	{
	InternalCrossReferenceResultSet *resultSet = new InternalCrossReferenceResultSet (this);

	try
		{
		resultSet->getCrossReference (primaryCatalog, primarySchema,primaryTable,foreignCatalog,foreignSchema, foreignTable);
		}
	catch (...)
		{
		delete resultSet;
		throw;
		}

	return resultSet;
	}

ResultSet* InternalDatabaseMetaData::getTypeInfo()
	{
	NOT_YET_IMPLEMENTED;
	return 0;
	}

bool InternalDatabaseMetaData::supportsResultSetConcurrency(int type, int concurrency)
	{
	NOT_YET_IMPLEMENTED;
	return 0;
	}

bool InternalDatabaseMetaData::ownUpdatesAreVisible(int type)
	{
	return true;
	}

bool InternalDatabaseMetaData::ownDeletesAreVisible(int type)
	{
	return true;
	}

bool InternalDatabaseMetaData::ownInsertsAreVisible(int type)
	{
	return true;
	}

bool InternalDatabaseMetaData::othersUpdatesAreVisible(int type)
	{
	return false;
	}

bool InternalDatabaseMetaData::othersDeletesAreVisible(int type)
	{
	return false;
	}

bool InternalDatabaseMetaData::othersInsertsAreVisible(int type)
	{
	return false;
	}

bool InternalDatabaseMetaData::updatesAreDetected(int type)
	{
	return false;
	}

bool InternalDatabaseMetaData::deletesAreDetected(int type)
	{
	return false;
	}

bool InternalDatabaseMetaData::insertsAreDetected(int type)
	{
	return false;
	}

bool InternalDatabaseMetaData::supportsBatchUpdates()
	{
	return false;
	}

ResultSet* InternalDatabaseMetaData::getUDTs(const char* catalog, const char* schemaPattern, 
		  const char* typeNamePattern, int* types)
	{
	NOT_YET_IMPLEMENTED;
	return 0;
	}


int InternalDatabaseMetaData::objectVersion()
{
	return DATABASEMETADATA_VERSION;
}
