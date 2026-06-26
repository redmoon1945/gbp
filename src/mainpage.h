/*
 *  Copyright (C) 2024-2026 Claude Dumas <claudedumas63@protonmail.com>. All rights reserved.
 *  DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/#AGPL/>.
 */

/**
 * @mainpage GBP (Graphical Budget Planner) - Architecture Overview
 *
 * @section intro Introduction
 * GBP is a personal finance management application for modeling and analyzing cash flows
 * over time. It uses a declarative approach where users define income/expense streams,
 * and the application generates concrete financial events for visualization and analysis.
 *
 * @section architecture Core Architecture
 *
 * The GBP domain model follows this flow:
 *
 * ```
 * User Input
 *    ↓
 * Scenario (contains multiple CSDs)
 *    ↓
 * Csd (Cash Stream Definition - abstract)
 *    ├─ PeriodicCsd (regular: daily, weekly, monthly, yearly)
 *    └─ IrregularCsd (irregular: specific dates)
 *    ↓
 * generateEventStream() → FeStream (collection of dated amounts)
 *    ↓
 * Fe (Financial Event - single dated amount)
 *    ↓
 * Visualization & Analysis
 * ```
 *
 * @section key_concepts Key Concepts
 *
 * - **Scenario:** Top-level container holding multiple CSDs and settings
 * - **Csd (Cash Stream Definition):** Declarative definition of income/expense pattern
 * - **Fe (Financial Event):** Concrete instance of money flow at a specific date
 * - **FeStream:** Ordered collection of FEs for a time period
 * - **Growth:** Pattern for increasing/decreasing amounts over time
 * - **Tags:** Categorization system for CSDs
 *
 * @section persistence Persistence & File Format
 *
 * GBP uses JSON for file persistence with version-aware serialization:
 * - Current version: 1.7.0
 * - Backward compatibility maintained via upgrade paths
 * - Each class implements toJsonObject() / fromJsonObject()
 * - Integer serialization for precise decimal handling
 *
 * @section important_classes Important Classes
 *
 * @subsection core_model Core Model
 * - Scenario - Top-level scenario container
 * - Csd - Abstract base for cash stream definitions
 * - PeriodicCsd - Regular recurring streams
 * - IrregularCsd - Irregular/one-time streams
 * - Fe - Single financial event
 * - FeStream - Financial event stream
 *
 * @subsection domain_logic Domain Logic
 * - Growth - Growth pattern application
 * - DateHelper - Date manipulation utilities
 * - DateRange - Date range handling
 * - Tag, Tags - Tagging/categorization system
 *
 * @subsection utilities Utilities
 * - Util - General utility functions
 * - CurrencyHelper - Currency formatting/parsing
 * - GbpLogger - Application logging
 * - GbpController - Application settings/state
 *
 * @section conventions Coding Conventions
 *
 * - Maximum line length: 100 characters
 * - Amounts stored as qint64 with 5 decimal places (e.g., 100000 = 1.00000)
 * - Dates use QDate, always validated
 * - Growth rates stored as integers in decimal form for precision
 * - UUIDs used for unique identification (not names)
 *
 * @section file_format JSON File Format Contract
 *
 * **Compatibility Requirements:**
 * - Integer fields for amounts maintain 5 decimal precision
 * - Enum values must maintain integer mappings across versions
 * - UUID strings must remain stable
 * - Date format: ISO 8601 (YYYY-MM-DD)
 * - Version field required for upgrade paths
 */

#ifndef MAINPAGE_H
#define MAINPAGE_H

// This file contains only Doxygen documentation for the main page.
// No actual code is defined here.

#endif // MAINPAGE_H
