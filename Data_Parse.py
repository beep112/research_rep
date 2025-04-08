import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import seaborn as sns

# Create a dataframe from your schedule data
# First, let's create the raw data from what was shown in the image
data = [
    [
        "8:00 AM",
        "Riya, Jason",
        "Riya, Jason, Anahaze",
        "Riya, Jason, Neal, Anahaze",
        "Riya, Jason, Anahaze",
        "Riya, Jason, Neal, Anahaze",
    ],
    [
        "9:00 AM",
        "Riya, Jason",
        "Jason, Anahaze",
        "Riya, Jason, Neal, Anahaze",
        "Riya, Jason, Anahaze",
        "Riya, Jason",
    ],
    [
        "10:00 AM",
        "Heidi",
        "Jason, Anahaze",
        "Jason, Heidi",
        "Lily, Riya, Jason, Anahaze",
        "Heidi, Caroline",
    ],
    [
        "11:00 AM",
        "Heidi",
        "Jason, Anahaze, Caroline",
        "Heidi, Anahaze",
        "Lily, Jason, Ciel, Caroline",
        "Heidi, Conner, Anahaze, Caroline",
    ],
    [
        "12:00 PM",
        "Jason, Heidi",
        "Riya, Jason, Anahaze, Caroline",
        "Lily, Jason, Heidi, Anahaze",
        "Jason, Caroline",
        "Jason, Heidi, Neal, Conner, Anahaze",
    ],
    [
        "1:00 PM",
        "Jason",
        "Riya, Jason, Anahaze, Caroline",
        "Lily, Jason, Heidi",
        "Jason, Neal",
        "Riya, Jason, Heidi, Conner",
    ],
    [
        "2:00 PM",
        "Riya, Heidi",
        "Lily, Riya, Jason",
        "Lily, Riya, Heidi, Anahaze",
        "Jason, Neal, Ciel, Anahaze",
        "Riya, Neal, Conner, Anahaze, Caroline",
    ],
    [
        "3:00 PM",
        "Riya, Jason, Heidi, Neal, Sydney, Caroline",
        "Lily, Riya",
        "Lily, Sydney, Anahaze, Caroline",
        "Neal, Ciel, Anahaze",
        "Neal, Anahaze, Caroline",
    ],
    [
        "4:00 PM",
        "Riya, Jason, Heidi, Neal, Ciel, Conner, Sydney, Caroline",
        "Lily, Riya",
        "Riya, Jason, Conner, Sydney, Caroline",
        "Riya (3:30), Neal, Ciel, Conner, Anahaze",
        "Jason, Neal, Anahaze, Caroline",
    ],
    [
        "5:00 PM",
        "Riya, Neal, Ciel, Conner, Sydney, Caroline",
        "Riya, Heidi, Neal, Conner, Sydney",
        "Riya, Neal, Conner, Sydney, Caroline",
        "Riya, Neal, Ciel, Conner, Anahaze",
        "Jason, Neal, Ciel, Sydney, Anahaze, Caroline",
    ],
    [
        "6:00 PM",
        "Riya, Neal, Ciel, Conner, Sydney, Caroline",
        "Riya, Heidi, Neal, Ciel, Conner, Sydney",
        "Riya, Jason, Heidi, Neal, Ciel, Conner, Sydney, Anahaze, Caroline",
        "Riya, Jason, Neal, Ciel, Conner, Sydney, Anahaze",
        "Lily, Jason, Neal, Ciel, Anahaze, Caroline",
    ],
    [
        "7:00 PM",
        "Lily, Riya, Jason, Neal, Conner, Sydney, Caroline",
        "Riya, Heidi, Neal, Ciel, Conner, Sydney",
        "Lily, Riya, Jason, Heidi, Neal, Conner, Sydney, Anahaze, Caroline",
        "Lily, Riya, Jason, Neal, Conner, Anahaze",
        "Jason, Neal, Anahaze",
    ],
    [
        "8:00 PM",
        "Lily, Riya, Jason, Neal, Ciel, Sydney",
        "Riya, Heidi, Neal, Ciel, Sydney",
        "Lily, Riya, Jason, Heidi, Neal, Sydney, Anahaze, Caroline",
        "Lily, Riya, Jason, Neal, Anahaze",
        "Jason, Neal, Anahaze",
    ],
]

# Column names
columns = ["Time", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday"]

# Create the dataframe
df = pd.DataFrame(data, columns=columns)

# Create a dictionary to store presence data for each person across time slots and days
people = [
    "Riya",
    "Jason",
    "Heidi",
    "Neal",
    "Lily",
    "Anahaze",
    "Caroline",
    "Conner",
    "Ciel",
    "Sydney",
]
presence_data = {}

# Process the data to extract presence information
for person in people:
    # Create a 2D array for each person (time slots x days)
    presence_array = np.zeros((len(df), 5))  # 5 days

    # Check presence in each cell
    for i, row in df.iterrows():
        for j, day in enumerate(columns[1:]):  # Skip the "Time" column
            if person in row[day]:
                presence_array[i, j] = 1

    presence_data[person] = presence_array

# Create heatmap for each person
plt.figure(figsize=(20, 15))

# Define subplot grid
n_people = len(people)
n_cols = 2
n_rows = (n_people + 1) // n_cols

# For storing aggregated data
all_presence = np.zeros((len(df), 5))

# Generate individual heatmaps
for i, person in enumerate(people):
    plt.subplot(n_rows, n_cols, i + 1)

    # Add data to aggregated view
    all_presence += presence_data[person]

    # Create heatmap
    ax = sns.heatmap(
        presence_data[person],
        cmap="YlGnBu",
        cbar=False,
        xticklabels=columns[1:],
        yticklabels=df["Time"],
        linewidths=0.5,
    )

    plt.title(f"{person}'s Schedule", fontsize=14)
    plt.tight_layout()

# Create the aggregated heatmap (showing total people per slot)
plt.figure(figsize=(12, 10))
ax = sns.heatmap(
    all_presence,
    cmap="YlOrRd",
    cbar=True,
    cbar_kws={"label": "Number of People"},
    xticklabels=columns[1:],
    yticklabels=df["Time"],
    linewidths=0.5,
    annot=True,
    fmt="g",
)

plt.title("Total People Present by Time Slot", fontsize=16)
plt.tight_layout()

# Create a "busiest hours" visualization
plt.figure(figsize=(12, 8))

# Calculate average attendance by hour
time_totals = all_presence.sum(axis=1)
plt.bar(df["Time"], time_totals)
plt.title("Busiest Hours (Average Attendance)", fontsize=16)
plt.xticks(rotation=45)
plt.ylabel("Total People")
plt.tight_layout()

# Show all plots
plt.show()
